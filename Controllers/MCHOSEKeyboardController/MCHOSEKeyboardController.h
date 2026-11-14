/*---------------------------------------------------------*\
| MCHOSEKeyboardController.h                                |
|                                                           |
|   Driver for MCHOSE keyboards (2.4G receiver protocol)    |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>
#include <hidapi.h>
#include "RGBController.h"

/*---------------------------------------------------------*\
| Wireless frame layout (HID report 0x13, 19 data bytes)    |
|   [cmd][total][index][length][14 payload bytes][checksum] |
|   checksum = (0x13 + sum of bytes 0..17) & 0xFF           |
| Reads are requested with [cmd, 0x01, 0x00...] and answer  |
| with `total` frames carrying the same command byte.       |
| Every written frame is echoed back by the receiver.       |
\*---------------------------------------------------------*/
#define MCHOSE_REPORT_ID                0x13
#define MCHOSE_REPORT_SIZE              20
#define MCHOSE_PAYLOAD_SIZE             14

enum
{
    MCHOSE_CMD_SET_PER_KEY              = 0x02,
    MCHOSE_CMD_SET_PERFORMANCE          = 0x04,
    MCHOSE_CMD_SET_LIGHT_COLOR          = 0x09,
    MCHOSE_CMD_GET_PER_KEY              = 0x42,
    MCHOSE_CMD_GET_PERFORMANCE          = 0x44,
    MCHOSE_CMD_GET_LIGHT_COLOR          = 0x49,
};

/*---------------------------------------------------------*\
| Lighting effect IDs (lightMode in the performance block)  |
\*---------------------------------------------------------*/
enum
{
    MCHOSE_MODE_OFF                     = 0x00,
    MCHOSE_MODE_STATIC                  = 0x01,
    MCHOSE_MODE_BREATHING               = 0x02,
    MCHOSE_MODE_RAINBOW                 = 0x03,
    MCHOSE_MODE_FLASH_AWAY              = 0x04,
    MCHOSE_MODE_RAINDROPS               = 0x05,
    MCHOSE_MODE_RIPPLE                  = 0x07,
    MCHOSE_MODE_STARS                   = 0x08,
    MCHOSE_MODE_FLOWING_WATER           = 0x0A,
    MCHOSE_MODE_TIDAL                   = 0x0B,
    MCHOSE_MODE_LIGHT_SHADOW            = 0x0C,
    MCHOSE_MODE_WAVE                    = 0x0D,
    MCHOSE_MODE_WINDMILL                = 0x0F,
    MCHOSE_MODE_WATERFALL               = 0x10,
    MCHOSE_MODE_FLOWER_DANCE            = 0x11,
    MCHOSE_MODE_CUSTOM                  = 0x15,
};

/*---------------------------------------------------------*\
| Performance block (128 bytes)                             |
|   [9]  lightType (1 = custom/per-key, 0 = effect)         |
|   [10] lightMode                                          |
|   per effect: [brightness][speed << 4 | multicolor]       |
|   [126..127] 0x5A 0xA5 terminator                         |
| Light color block (490 bytes): effect N color at 21 * N   |
| Per-key block (378 bytes): 126 R, then 126 G, then 126 B  |
\*---------------------------------------------------------*/
#define MCHOSE_PERFORMANCE_SIZE         128
#define MCHOSE_LIGHT_COLOR_SIZE         490
#define MCHOSE_PER_KEY_LEDS             126
#define MCHOSE_PER_KEY_SIZE             (3 * MCHOSE_PER_KEY_LEDS)

#define MCHOSE_PERF_LIGHT_TYPE          9
#define MCHOSE_PERF_LIGHT_MODE          10

#define MCHOSE_BRIGHTNESS_MIN           1
#define MCHOSE_BRIGHTNESS_MAX           9
#define MCHOSE_SPEED_MIN                1
#define MCHOSE_SPEED_MAX                4
#define MCHOSE_MULTICOLOR_ON            7

class MCHOSEKeyboardController
{
public:
    MCHOSEKeyboardController(hid_device* dev_handle, const char* path, std::string dev_name);
    ~MCHOSEKeyboardController();

    std::string GetDeviceLocation();
    std::string GetDeviceName();
    std::string GetSerialString();

    bool        ReadState();
    bool        IsStateValid();

    unsigned char   GetActiveMode();
    unsigned char   GetModeBrightness(unsigned char mode);
    unsigned char   GetModeSpeed(unsigned char mode);
    bool            GetModeRandom(unsigned char mode);
    RGBColor        GetModeColor(unsigned char mode);

    void        SetMode(unsigned char mode, unsigned char brightness, unsigned char speed, bool random);
    void        SetModeColor(unsigned char mode, RGBColor color);
    void        SetPerKeyColors(const RGBColor* colors);

private:
    hid_device*     dev;
    std::string     location;
    std::string     name;

    bool                        state_valid;
    std::vector<unsigned char>  performance;
    std::vector<unsigned char>  light_color;

    int         GetModePairOffset(unsigned char mode);

    void        SendFrame(unsigned char cmd, unsigned char total, unsigned char index, const unsigned char* payload, unsigned char length);
    bool        ReadBlock(unsigned char cmd, std::vector<unsigned char>& out);
    void        WriteBlock(unsigned char cmd, const std::vector<unsigned char>& data, bool trim_last);
};
