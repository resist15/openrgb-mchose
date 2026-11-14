/*---------------------------------------------------------*\
| MCHOSEKeyboardController.cpp                              |
|                                                           |
|   Driver for MCHOSE keyboards (2.4G receiver protocol)    |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <algorithm>
#include <chrono>
#include <cstring>
#include "MCHOSEKeyboardController.h"
#include "StringUtils.h"

#define MCHOSE_READ_TIMEOUT_MS          1000
#define MCHOSE_ECHO_TIMEOUT_MS          100

MCHOSEKeyboardController::MCHOSEKeyboardController(hid_device* dev_handle, const char* path, std::string dev_name)
{
    dev         = dev_handle;
    location    = path;
    name        = dev_name;
    state_valid = false;

    ReadState();
}

MCHOSEKeyboardController::~MCHOSEKeyboardController()
{
    hid_close(dev);
}

std::string MCHOSEKeyboardController::GetDeviceLocation()
{
    return("HID: " + location);
}

std::string MCHOSEKeyboardController::GetDeviceName()
{
    return(name);
}

std::string MCHOSEKeyboardController::GetSerialString()
{
    wchar_t serial_string[128];
    int     ret = hid_get_serial_number_string(dev, serial_string, 128);

    if(ret != 0)
    {
        return("");
    }

    return(StringUtils::wstring_to_string(serial_string));
}

/*---------------------------------------------------------*\
| Read the performance and effect color blocks so that      |
| writes preserve every non-lighting setting (polling rate, |
| sleep timer, etc.) stored in the same blocks.             |
\*---------------------------------------------------------*/
bool MCHOSEKeyboardController::ReadState()
{
    std::vector<unsigned char> new_performance;
    std::vector<unsigned char> new_light_color;

    state_valid = ReadBlock(MCHOSE_CMD_GET_PERFORMANCE, new_performance)
               && ReadBlock(MCHOSE_CMD_GET_LIGHT_COLOR, new_light_color)
               && new_performance.size() == MCHOSE_PERFORMANCE_SIZE
               && new_light_color.size() == MCHOSE_LIGHT_COLOR_SIZE;

    if(state_valid)
    {
        performance = new_performance;
        light_color = new_light_color;
    }

    return(state_valid);
}

bool MCHOSEKeyboardController::IsStateValid()
{
    return(state_valid);
}

int MCHOSEKeyboardController::GetModePairOffset(unsigned char mode)
{
    switch(mode)
    {
        case MCHOSE_MODE_STATIC:        return(58);
        case MCHOSE_MODE_BREATHING:     return(60);
        case MCHOSE_MODE_RAINBOW:       return(62);
        case MCHOSE_MODE_FLASH_AWAY:    return(64);
        case MCHOSE_MODE_RAINDROPS:     return(66);
        case MCHOSE_MODE_RIPPLE:        return(70);
        case MCHOSE_MODE_STARS:         return(72);
        case MCHOSE_MODE_FLOWING_WATER: return(76);
        case MCHOSE_MODE_TIDAL:         return(78);
        case MCHOSE_MODE_LIGHT_SHADOW:  return(80);
        case MCHOSE_MODE_WAVE:          return(82);
        case MCHOSE_MODE_WINDMILL:      return(84);
        case MCHOSE_MODE_WATERFALL:     return(88);
        case MCHOSE_MODE_FLOWER_DANCE:  return(90);
        default:                        return(-1);
    }
}

unsigned char MCHOSEKeyboardController::GetActiveMode()
{
    return(state_valid ? performance[MCHOSE_PERF_LIGHT_MODE] : (unsigned char)MCHOSE_MODE_CUSTOM);
}

unsigned char MCHOSEKeyboardController::GetModeBrightness(unsigned char mode)
{
    int offset = GetModePairOffset(mode);

    if(!state_valid || offset < 0)
    {
        return(MCHOSE_BRIGHTNESS_MAX);
    }

    unsigned char brightness = performance[offset];

    if(brightness < MCHOSE_BRIGHTNESS_MIN || brightness > MCHOSE_BRIGHTNESS_MAX)
    {
        brightness = MCHOSE_BRIGHTNESS_MAX;
    }

    return(brightness);
}

unsigned char MCHOSEKeyboardController::GetModeSpeed(unsigned char mode)
{
    int offset = GetModePairOffset(mode);

    if(!state_valid || offset < 0)
    {
        return(3);
    }

    unsigned char speed = performance[offset + 1] >> 4;

    if(speed < MCHOSE_SPEED_MIN || speed > MCHOSE_SPEED_MAX)
    {
        speed = 3;
    }

    return(speed);
}

bool MCHOSEKeyboardController::GetModeRandom(unsigned char mode)
{
    int offset = GetModePairOffset(mode);

    if(!state_valid || offset < 0)
    {
        return(false);
    }

    return((performance[offset + 1] & 0x0F) == MCHOSE_MULTICOLOR_ON);
}

RGBColor MCHOSEKeyboardController::GetModeColor(unsigned char mode)
{
    unsigned int offset = 21 * mode;

    if(!state_valid || offset + 3 > light_color.size())
    {
        return(ToRGBColor(0xFF, 0x00, 0x00));
    }

    return(ToRGBColor(light_color[offset], light_color[offset + 1], light_color[offset + 2]));
}

void MCHOSEKeyboardController::SetMode(unsigned char mode, unsigned char brightness, unsigned char speed, bool random)
{
    if(!state_valid && !ReadState())
    {
        return;
    }

    performance[MCHOSE_PERF_LIGHT_TYPE] = (mode == MCHOSE_MODE_CUSTOM) ? 1 : 0;
    performance[MCHOSE_PERF_LIGHT_MODE] = mode;

    int offset = GetModePairOffset(mode);

    if(offset >= 0)
    {
        performance[offset]     = brightness;
        performance[offset + 1] = (unsigned char)((speed << 4) | (random ? MCHOSE_MULTICOLOR_ON : 0));
    }

    performance[MCHOSE_PERFORMANCE_SIZE - 2] = 0x5A;
    performance[MCHOSE_PERFORMANCE_SIZE - 1] = 0xA5;

    WriteBlock(MCHOSE_CMD_SET_PERFORMANCE, performance, false);
}

void MCHOSEKeyboardController::SetModeColor(unsigned char mode, RGBColor color)
{
    if(!state_valid && !ReadState())
    {
        return;
    }

    unsigned int offset = 21 * mode;

    if(offset + 3 > light_color.size())
    {
        return;
    }

    light_color[offset]     = RGBGetRValue(color);
    light_color[offset + 1] = RGBGetGValue(color);
    light_color[offset + 2] = RGBGetBValue(color);

    /*-----------------------------------------------------*\
    | The write is the color block followed by 14 zero      |
    | bytes and a 0x5A 0xA5 terminator block                |
    \*-----------------------------------------------------*/
    std::vector<unsigned char> data(light_color);
    data.resize(light_color.size() + 2 * MCHOSE_PAYLOAD_SIZE, 0x00);
    data[light_color.size() + MCHOSE_PAYLOAD_SIZE + 2] = 0x5A;
    data[light_color.size() + MCHOSE_PAYLOAD_SIZE + 3] = 0xA5;

    WriteBlock(MCHOSE_CMD_SET_LIGHT_COLOR, data, true);
}

void MCHOSEKeyboardController::SetPerKeyColors(const RGBColor* colors)
{
    std::vector<unsigned char> data(MCHOSE_PER_KEY_SIZE);

    for(unsigned int led_idx = 0; led_idx < MCHOSE_PER_KEY_LEDS; led_idx++)
    {
        data[led_idx]                               = RGBGetRValue(colors[led_idx]);
        data[led_idx + MCHOSE_PER_KEY_LEDS]         = RGBGetGValue(colors[led_idx]);
        data[led_idx + 2 * MCHOSE_PER_KEY_LEDS]     = RGBGetBValue(colors[led_idx]);
    }

    WriteBlock(MCHOSE_CMD_SET_PER_KEY, data, false);
}

void MCHOSEKeyboardController::SendFrame(unsigned char cmd, unsigned char total, unsigned char index, const unsigned char* payload, unsigned char length)
{
    unsigned char buf[MCHOSE_REPORT_SIZE];

    memset(buf, 0x00, sizeof(buf));

    buf[0] = MCHOSE_REPORT_ID;
    buf[1] = cmd;
    buf[2] = total;
    buf[3] = index;
    buf[4] = length;

    if(length > 0)
    {
        memcpy(&buf[5], payload, length);
    }

    unsigned char checksum = 0;

    for(unsigned int i = 0; i < MCHOSE_REPORT_SIZE - 1; i++)
    {
        checksum += buf[i];
    }

    buf[MCHOSE_REPORT_SIZE - 1] = checksum;

    hid_write(dev, buf, MCHOSE_REPORT_SIZE);
}

bool MCHOSEKeyboardController::ReadBlock(unsigned char cmd, std::vector<unsigned char>& out)
{
    unsigned char buf[64];

    /*-----------------------------------------------------*\
    | Flush pending input reports                           |
    \*-----------------------------------------------------*/
    while(hid_read_timeout(dev, buf, sizeof(buf), 0) > 0)
    {
    }

    SendFrame(cmd, 0x01, 0x00, NULL, 0);

    std::vector<std::vector<unsigned char>> blocks;
    unsigned int                            received = 0;
    auto                                    start    = std::chrono::steady_clock::now();

    while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(MCHOSE_READ_TIMEOUT_MS))
    {
        int len = hid_read_timeout(dev, buf, sizeof(buf), 50);

        if(len < MCHOSE_REPORT_SIZE || buf[0] != MCHOSE_REPORT_ID || buf[1] != cmd)
        {
            continue;
        }

        unsigned char total  = buf[2];
        unsigned char index  = buf[3];
        unsigned char length = buf[4];

        if(total == 0 || index >= total || length > MCHOSE_PAYLOAD_SIZE)
        {
            continue;
        }

        if(blocks.size() != total)
        {
            blocks.assign(total, std::vector<unsigned char>());
            received = 0;
        }

        if(blocks[index].empty() && length > 0)
        {
            received++;
        }

        blocks[index].assign(&buf[5], &buf[5] + length);

        if(received == total)
        {
            out.clear();

            for(unsigned int block_idx = 0; block_idx < blocks.size(); block_idx++)
            {
                out.insert(out.end(), blocks[block_idx].begin(), blocks[block_idx].end());
            }

            return(true);
        }
    }

    return(false);
}

void MCHOSEKeyboardController::WriteBlock(unsigned char cmd, const std::vector<unsigned char>& data, bool trim_last)
{
    unsigned char   buf[64];
    unsigned int    total = (unsigned int)((data.size() + MCHOSE_PAYLOAD_SIZE - 1) / MCHOSE_PAYLOAD_SIZE);

    while(hid_read_timeout(dev, buf, sizeof(buf), 0) > 0)
    {
    }

    for(unsigned int index = 0; index < total; index++)
    {
        unsigned int offset = index * MCHOSE_PAYLOAD_SIZE;
        unsigned int length = (unsigned int)std::min<size_t>(MCHOSE_PAYLOAD_SIZE, data.size() - offset);

        /*-------------------------------------------------*\
        | Some blocks use the trailing non-zero length for  |
        | the final frame                                   |
        \*-------------------------------------------------*/
        if(trim_last && index == total - 1)
        {
            while(length > 0 && data[offset + length - 1] == 0x00)
            {
                length--;
            }
        }

        SendFrame(cmd, (unsigned char)total, (unsigned char)index, &data[offset], (unsigned char)length);

        /*-------------------------------------------------*\
        | Wait for the receiver to echo the frame before    |
        | sending the next one                              |
        \*-------------------------------------------------*/
        auto start = std::chrono::steady_clock::now();

        while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(MCHOSE_ECHO_TIMEOUT_MS))
        {
            int len = hid_read_timeout(dev, buf, sizeof(buf), 20);

            if(len >= MCHOSE_REPORT_SIZE && buf[0] == MCHOSE_REPORT_ID && buf[1] == cmd && buf[3] == index)
            {
                break;
            }
        }
    }
}
