/*---------------------------------------------------------*\
| MCHOSEKeyboardController.cpp                              |
|                                                           |
|   Driver for MCHOSE keyboards (2.4G receiver and wired)   |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <chrono>
#include <cstring>
#include <thread>
#include "MCHOSEKeyboardController.h"
#include "StringUtils.h"

#define MCHOSE_READ_TIMEOUT_MS          1000
#define MCHOSE_ECHO_TIMEOUT_MS          100
#define MCHOSE_WIRED_WRITE_DELAY_MS     50

MCHOSEKeyboardController::MCHOSEKeyboardController(hid_device* dev_handle, const char* path, std::string dev_name, bool is_wired)
{
    dev         = dev_handle;
    location    = path;
    name        = dev_name;
    wired       = is_wired;
    state_valid = false;

    memset(performance,         0x00, sizeof(performance));
    memset(light_color,         0x00, sizeof(light_color));
    memset(performance_header,  0x00, sizeof(performance_header));
    memset(light_color_header,  0x00, sizeof(light_color_header));

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
    unsigned char new_performance[MCHOSE_PERFORMANCE_SIZE];
    unsigned char new_light_color[MCHOSE_WIRED_LIGHT_COLOR_SIZE];
    unsigned char new_performance_header[MCHOSE_WIRED_HEADER_SIZE];
    unsigned char new_light_color_header[MCHOSE_WIRED_HEADER_SIZE];

    memset(new_light_color,         0x00, sizeof(new_light_color));
    memset(new_performance_header,  0x00, sizeof(new_performance_header));
    memset(new_light_color_header,  0x00, sizeof(new_light_color_header));

    bool success;

    if(wired)
    {
        success = WiredRead(MCHOSE_WIRED_CMD_PERFORMANCE, MCHOSE_PERFORMANCE_SIZE, new_performance_header, new_performance)
               && WiredRead(MCHOSE_WIRED_CMD_LIGHT_COLOR, MCHOSE_WIRED_LIGHT_COLOR_SIZE, new_light_color_header, new_light_color);
    }
    else
    {
        success = ReadBlock(MCHOSE_CMD_GET_PERFORMANCE, new_performance, MCHOSE_PERFORMANCE_SIZE)
               && ReadBlock(MCHOSE_CMD_GET_LIGHT_COLOR, new_light_color, MCHOSE_LIGHT_COLOR_SIZE);
    }

    if(success)
    {
        memcpy(performance,         new_performance,        sizeof(performance));
        memcpy(light_color,         new_light_color,        sizeof(light_color));
        memcpy(performance_header,  new_performance_header, sizeof(performance_header));
        memcpy(light_color_header,  new_light_color_header, sizeof(light_color_header));
    }

    state_valid = success;

    return(state_valid);
}

bool MCHOSEKeyboardController::IsStateValid()
{
    return(state_valid);
}

bool MCHOSEKeyboardController::IsWired()
{
    return(wired);
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
        return(MCHOSE_SPEED_DEFAULT);
    }

    unsigned char speed = performance[offset + 1] >> 4;

    if(speed < MCHOSE_SPEED_MIN || speed > MCHOSE_SPEED_MAX)
    {
        speed = MCHOSE_SPEED_DEFAULT;
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
    unsigned int offset = MCHOSE_MODE_COLOR_STRIDE * mode;

    if(!state_valid || offset + 3 > MCHOSE_LIGHT_COLOR_SIZE)
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

    if(wired)
    {
        WiredWrite(MCHOSE_WIRED_CMD_PERFORMANCE, performance_header, performance, MCHOSE_PERFORMANCE_SIZE);
    }
    else
    {
        WriteBlock(MCHOSE_CMD_SET_PERFORMANCE, performance, MCHOSE_PERFORMANCE_SIZE, false);
    }
}

void MCHOSEKeyboardController::SetModeColor(unsigned char mode, RGBColor color)
{
    if(!state_valid && !ReadState())
    {
        return;
    }

    unsigned int offset = MCHOSE_MODE_COLOR_STRIDE * mode;

    if(offset + 3 > MCHOSE_LIGHT_COLOR_SIZE)
    {
        return;
    }

    light_color[offset]     = RGBGetRValue(color);
    light_color[offset + 1] = RGBGetGValue(color);
    light_color[offset + 2] = RGBGetBValue(color);

    /*-----------------------------------------------------*\
    | Wired: write back the whole block as it was read,     |
    | which already ends with the 0x5A 0xA5 terminator      |
    \*-----------------------------------------------------*/
    if(wired)
    {
        WiredWrite(MCHOSE_WIRED_CMD_LIGHT_COLOR, light_color_header, light_color, MCHOSE_WIRED_LIGHT_COLOR_SIZE);
        return;
    }

    /*-----------------------------------------------------*\
    | Wireless: the color block followed by 14 zero bytes   |
    | and a 0x5A 0xA5 terminator block                      |
    \*-----------------------------------------------------*/
    unsigned char data[MCHOSE_LIGHT_COLOR_SIZE + 2 * MCHOSE_PAYLOAD_SIZE];

    memset(data, 0x00, sizeof(data));
    memcpy(data, light_color, MCHOSE_LIGHT_COLOR_SIZE);

    data[MCHOSE_LIGHT_COLOR_SIZE + MCHOSE_PAYLOAD_SIZE + 2] = 0x5A;
    data[MCHOSE_LIGHT_COLOR_SIZE + MCHOSE_PAYLOAD_SIZE + 3] = 0xA5;

    WriteBlock(MCHOSE_CMD_SET_LIGHT_COLOR, data, sizeof(data), true);
}

void MCHOSEKeyboardController::SetPerKeyColors(const RGBColor* colors)
{
    unsigned char data[MCHOSE_PER_KEY_SIZE];

    for(unsigned int led_idx = 0; led_idx < MCHOSE_PER_KEY_LEDS; led_idx++)
    {
        data[led_idx]                               = RGBGetRValue(colors[led_idx]);
        data[led_idx + MCHOSE_PER_KEY_LEDS]         = RGBGetGValue(colors[led_idx]);
        data[led_idx + 2 * MCHOSE_PER_KEY_LEDS]     = RGBGetBValue(colors[led_idx]);
    }

    if(wired)
    {
        const unsigned char per_key_header[MCHOSE_WIRED_HEADER_SIZE] = { 0x00, 0x00, 0x01, 0x00, 0x80, 0x01 };

        WiredWrite(MCHOSE_WIRED_CMD_PER_KEY, per_key_header, data, MCHOSE_PER_KEY_SIZE);
    }
    else
    {
        WriteBlock(MCHOSE_CMD_SET_PER_KEY, data, MCHOSE_PER_KEY_SIZE, false);
    }
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

bool MCHOSEKeyboardController::ReadBlock(unsigned char cmd, unsigned char* out, unsigned int size)
{
    unsigned char   buf[64];
    bool            block_received[MCHOSE_MAX_BLOCKS];
    unsigned int    blocks_total    = 0;
    unsigned int    blocks_received = 0;
    unsigned int    bytes_received  = 0;

    memset(block_received, 0, sizeof(block_received));

    /*-----------------------------------------------------*\
    | Flush pending input reports                           |
    \*-----------------------------------------------------*/
    while(hid_read_timeout(dev, buf, sizeof(buf), 0) > 0)
    {
    }

    SendFrame(cmd, 0x01, 0x00, NULL, 0);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(MCHOSE_READ_TIMEOUT_MS))
    {
        int len = hid_read_timeout(dev, buf, sizeof(buf), 50);

        if(len < MCHOSE_REPORT_SIZE || buf[0] != MCHOSE_REPORT_ID || buf[1] != cmd)
        {
            continue;
        }

        unsigned int total  = buf[2];
        unsigned int index  = buf[3];
        unsigned int length = buf[4];
        unsigned int offset = index * MCHOSE_PAYLOAD_SIZE;

        if(total == 0 || total > MCHOSE_MAX_BLOCKS || index >= total || length > MCHOSE_PAYLOAD_SIZE || offset + length > size)
        {
            continue;
        }

        if(blocks_total != total)
        {
            memset(block_received, 0, sizeof(block_received));
            blocks_total    = total;
            blocks_received = 0;
            bytes_received  = 0;
        }

        if(!block_received[index])
        {
            memcpy(&out[offset], &buf[5], length);
            block_received[index]   = true;
            blocks_received++;
            bytes_received         += length;
        }

        if(blocks_received == blocks_total)
        {
            return(bytes_received == size);
        }
    }

    return(false);
}

void MCHOSEKeyboardController::WriteBlock(unsigned char cmd, const unsigned char* data, unsigned int size, bool trim_last)
{
    unsigned char   buf[64];
    unsigned int    total = (size + MCHOSE_PAYLOAD_SIZE - 1) / MCHOSE_PAYLOAD_SIZE;

    while(hid_read_timeout(dev, buf, sizeof(buf), 0) > 0)
    {
    }

    for(unsigned int index = 0; index < total; index++)
    {
        unsigned int offset = index * MCHOSE_PAYLOAD_SIZE;
        unsigned int length = size - offset;

        if(length > MCHOSE_PAYLOAD_SIZE)
        {
            length = MCHOSE_PAYLOAD_SIZE;
        }

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
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

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

bool MCHOSEKeyboardController::WiredRead(unsigned char cmd, unsigned int size, unsigned char* header, unsigned char* out)
{
    unsigned char buf[MCHOSE_WIRED_REPORT_SIZE];

    if(size > MCHOSE_WIRED_REPORT_SIZE - MCHOSE_WIRED_DATA_OFFSET)
    {
        return(false);
    }

    memset(buf, 0x00, sizeof(buf));

    buf[0] = MCHOSE_WIRED_REPORT_ID;
    buf[1] = cmd | MCHOSE_WIRED_READ_FLAG;
    buf[4] = 0x01;
    buf[6] = size & 0xFF;
    buf[7] = (size >> 8) & 0xFF;

    if(hid_send_feature_report(dev, buf, sizeof(buf)) < 0)
    {
        return(false);
    }

    memset(buf, 0x00, sizeof(buf));
    buf[0] = MCHOSE_WIRED_REPORT_ID;

    int received = hid_get_feature_report(dev, buf, sizeof(buf));

    if(received < (int)(MCHOSE_WIRED_DATA_OFFSET + size) || buf[1] != (cmd | MCHOSE_WIRED_READ_FLAG))
    {
        return(false);
    }

    memcpy(header, &buf[2], MCHOSE_WIRED_HEADER_SIZE);
    memcpy(out, &buf[MCHOSE_WIRED_DATA_OFFSET], size);

    return(true);
}

void MCHOSEKeyboardController::WiredWrite(unsigned char cmd, const unsigned char* header, const unsigned char* data, unsigned int size)
{
    unsigned char buf[MCHOSE_WIRED_REPORT_SIZE];

    if(size > MCHOSE_WIRED_REPORT_SIZE - MCHOSE_WIRED_DATA_OFFSET)
    {
        return;
    }

    memset(buf, 0x00, sizeof(buf));

    buf[0] = MCHOSE_WIRED_REPORT_ID;
    buf[1] = cmd;
    memcpy(&buf[2], header, MCHOSE_WIRED_HEADER_SIZE);
    memcpy(&buf[MCHOSE_WIRED_DATA_OFFSET], data, size);

    hid_send_feature_report(dev, buf, sizeof(buf));

    /*-----------------------------------------------------*\
    | The keyboard resets if a per-key write arrives right  |
    | after switching to custom mode, so give it time to    |
    | process each write                                    |
    \*-----------------------------------------------------*/
    std::this_thread::sleep_for(std::chrono::milliseconds(MCHOSE_WIRED_WRITE_DELAY_MS));
}
