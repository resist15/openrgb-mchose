/*---------------------------------------------------------*\
| RGBController_MCHOSEKeyboard.cpp                          |
|                                                           |
|   RGBController for MCHOSE keyboards                      |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBControllerKeyNames.h"
#include "RGBController_MCHOSEKeyboard.h"

/*---------------------------------------------------------*\
| G75 Pro key layout                                        |
|   index = position in the per-key color tables, which the |
|   firmware orders column by column, 6 rows per column     |
\*---------------------------------------------------------*/
#define MCHOSE_G75_PRO_ROWS     6
#define MCHOSE_G75_PRO_COLS     16

struct mchose_key
{
    unsigned int    index;
    unsigned int    row;
    unsigned int    col;
    const char*     name;
};

static const mchose_key mchose_g75_pro_keys[] =
{
    {   0,  0,  0,  KEY_EN_ESCAPE           },
    {   6,  0,  1,  KEY_EN_F1               },
    {  12,  0,  2,  KEY_EN_F2               },
    {  18,  0,  3,  KEY_EN_F3               },
    {  24,  0,  4,  KEY_EN_F4               },
    {  30,  0,  5,  KEY_EN_F5               },
    {  36,  0,  6,  KEY_EN_F6               },
    {  42,  0,  7,  KEY_EN_F7               },
    {  48,  0,  8,  KEY_EN_F8               },
    {  54,  0,  9,  KEY_EN_F9               },
    {  60,  0, 10,  KEY_EN_F10              },
    {  66,  0, 11,  KEY_EN_F11              },
    {  72,  0, 12,  KEY_EN_F12              },
    {  78,  0, 14,  KEY_EN_INSERT           },
    {  90,  0, 15,  KEY_EN_DELETE           },

    {   1,  1,  0,  KEY_EN_BACK_TICK        },
    {   7,  1,  1,  KEY_EN_1                },
    {  13,  1,  2,  KEY_EN_2                },
    {  19,  1,  3,  KEY_EN_3                },
    {  25,  1,  4,  KEY_EN_4                },
    {  31,  1,  5,  KEY_EN_5                },
    {  37,  1,  6,  KEY_EN_6                },
    {  43,  1,  7,  KEY_EN_7                },
    {  49,  1,  8,  KEY_EN_8                },
    {  55,  1,  9,  KEY_EN_9                },
    {  61,  1, 10,  KEY_EN_0                },
    {  67,  1, 11,  KEY_EN_MINUS            },
    {  73,  1, 12,  KEY_EN_EQUALS           },
    {  79,  1, 13,  KEY_EN_BACKSPACE        },
    {  91,  1, 15,  KEY_EN_HOME             },

    {   2,  2,  0,  KEY_EN_TAB              },
    {   8,  2,  1,  KEY_EN_Q                },
    {  14,  2,  2,  KEY_EN_W                },
    {  20,  2,  3,  KEY_EN_E                },
    {  26,  2,  4,  KEY_EN_R                },
    {  32,  2,  5,  KEY_EN_T                },
    {  38,  2,  6,  KEY_EN_Y                },
    {  44,  2,  7,  KEY_EN_U                },
    {  50,  2,  8,  KEY_EN_I                },
    {  56,  2,  9,  KEY_EN_O                },
    {  62,  2, 10,  KEY_EN_P                },
    {  68,  2, 11,  KEY_EN_LEFT_BRACKET     },
    {  74,  2, 12,  KEY_EN_RIGHT_BRACKET    },
    {  80,  2, 13,  KEY_EN_ANSI_BACK_SLASH  },
    {  92,  2, 15,  KEY_EN_PAGE_UP          },

    {   3,  3,  0,  KEY_EN_CAPS_LOCK        },
    {   9,  3,  1,  KEY_EN_A                },
    {  15,  3,  2,  KEY_EN_S                },
    {  21,  3,  3,  KEY_EN_D                },
    {  27,  3,  4,  KEY_EN_F                },
    {  33,  3,  5,  KEY_EN_G                },
    {  39,  3,  6,  KEY_EN_H                },
    {  45,  3,  7,  KEY_EN_J                },
    {  51,  3,  8,  KEY_EN_K                },
    {  57,  3,  9,  KEY_EN_L                },
    {  63,  3, 10,  KEY_EN_SEMICOLON        },
    {  69,  3, 11,  KEY_EN_QUOTE            },
    {  81,  3, 13,  KEY_EN_ANSI_ENTER       },
    {  93,  3, 15,  KEY_EN_PAGE_DOWN        },

    {   4,  4,  0,  KEY_EN_LEFT_SHIFT       },
    {  10,  4,  2,  KEY_EN_Z                },
    {  16,  4,  3,  KEY_EN_X                },
    {  22,  4,  4,  KEY_EN_C                },
    {  28,  4,  5,  KEY_EN_V                },
    {  34,  4,  6,  KEY_EN_B                },
    {  40,  4,  7,  KEY_EN_N                },
    {  46,  4,  8,  KEY_EN_M                },
    {  52,  4,  9,  KEY_EN_COMMA            },
    {  58,  4, 10,  KEY_EN_PERIOD           },
    {  64,  4, 11,  KEY_EN_FORWARD_SLASH    },
    {  82,  4, 12,  KEY_EN_RIGHT_SHIFT      },
    {  88,  4, 14,  KEY_EN_UP_ARROW         },
    {  94,  4, 15,  KEY_EN_END              },

    {   5,  5,  0,  KEY_EN_LEFT_CONTROL     },
    {  11,  5,  1,  KEY_EN_LEFT_WINDOWS     },
    {  17,  5,  2,  KEY_EN_LEFT_ALT         },
    {  35,  5,  6,  KEY_EN_SPACE            },
    {  53,  5, 10,  KEY_EN_RIGHT_FUNCTION   },
    {  59,  5, 11,  KEY_EN_RIGHT_CONTROL    },
    {  83,  5, 13,  KEY_EN_LEFT_ARROW       },
    {  89,  5, 14,  KEY_EN_DOWN_ARROW       },
    {  95,  5, 15,  KEY_EN_RIGHT_ARROW      },
};

/**------------------------------------------------------------------*\
    @name MCHOSE G75 Pro
    @category Keyboard
    @type USB
    @save :white_check_mark:
    @direct :x:
    @effects :white_check_mark:
    @detectors DetectMCHOSEKeyboard, DetectSinowealthKeyboard10c
    @comment Supported over the 2.4G wireless receiver and USB cable.
        Custom mode writes the keyboard's stored per-key lighting table.
        While charging over USB, the Esc key shows the charge indicator.
\*-------------------------------------------------------------------*/

RGBController_MCHOSEKeyboard::RGBController_MCHOSEKeyboard(MCHOSEKeyboardController* controller_ptr, std::function<void()> destroy_callback)
{
    controller          = controller_ptr;
    on_destroy          = destroy_callback;

    name                = controller->GetDeviceName();
    vendor              = "MCHOSE";
    type                = DEVICE_TYPE_KEYBOARD;
    description         = "MCHOSE Keyboard Device";
    location            = controller->GetDeviceLocation();
    serial              = controller->GetSerialString();

    mode Custom;
    Custom.name         = "Custom";
    Custom.value        = MCHOSE_MODE_CUSTOM;
    Custom.flags        = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_AUTOMATIC_SAVE;
    Custom.color_mode   = MODE_COLORS_PER_LED;
    modes.push_back(Custom);

    AddEffectMode("Static",           MCHOSE_MODE_STATIC,        true,  false, true );
    AddEffectMode("Breathing",        MCHOSE_MODE_BREATHING,     true,  true,  true );
    AddEffectMode("Rainbow",          MCHOSE_MODE_RAINBOW,       true,  true,  false);
    AddEffectMode("Flash Away",       MCHOSE_MODE_FLASH_AWAY,    true,  true,  true );
    AddEffectMode("Raindrops",        MCHOSE_MODE_RAINDROPS,     true,  true,  true );
    AddEffectMode("Ripple",           MCHOSE_MODE_RIPPLE,        true,  true,  true );
    AddEffectMode("Stars",            MCHOSE_MODE_STARS,         true,  true,  true );
    AddEffectMode("Flowing Water",    MCHOSE_MODE_FLOWING_WATER, true,  true,  true );
    AddEffectMode("Tidal",            MCHOSE_MODE_TIDAL,         true,  true,  true );
    AddEffectMode("Light and Shadow", MCHOSE_MODE_LIGHT_SHADOW,  true,  true,  true );
    AddEffectMode("Wave",             MCHOSE_MODE_WAVE,          true,  true,  true );
    AddEffectMode("Windmill",         MCHOSE_MODE_WINDMILL,      false, false, false);
    AddEffectMode("Waterfall",        MCHOSE_MODE_WATERFALL,     true,  true,  false);
    AddEffectMode("Flower Dance",     MCHOSE_MODE_FLOWER_DANCE,  false, false, false);

    mode Off;
    Off.name            = "Off";
    Off.value           = MCHOSE_MODE_OFF;
    Off.flags           = MODE_FLAG_AUTOMATIC_SAVE;
    Off.color_mode      = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();

    /*-----------------------------------------------------*\
    | Reflect the effect currently stored on the keyboard   |
    \*-----------------------------------------------------*/
    unsigned char current_mode = controller->GetActiveMode();

    for(unsigned int mode_idx = 0; mode_idx < modes.size(); mode_idx++)
    {
        if(modes[mode_idx].value == current_mode)
        {
            active_mode = mode_idx;
            break;
        }
    }
}

RGBController_MCHOSEKeyboard::~RGBController_MCHOSEKeyboard()
{
    Shutdown();

    delete controller;

    if(on_destroy)
    {
        on_destroy();
    }
}

void RGBController_MCHOSEKeyboard::AddEffectMode(const char* mode_name, unsigned char value, bool has_brightness, bool has_speed, bool has_color)
{
    mode new_mode;

    new_mode.name           = mode_name;
    new_mode.value          = value;
    new_mode.flags          = MODE_FLAG_AUTOMATIC_SAVE;
    new_mode.color_mode     = MODE_COLORS_NONE;

    if(has_brightness)
    {
        new_mode.flags         |= MODE_FLAG_HAS_BRIGHTNESS;
        new_mode.brightness_min = MCHOSE_BRIGHTNESS_MIN;
        new_mode.brightness_max = MCHOSE_BRIGHTNESS_MAX;
        new_mode.brightness     = controller->GetModeBrightness(value);
    }

    if(has_speed)
    {
        new_mode.flags         |= MODE_FLAG_HAS_SPEED;
        new_mode.speed_min      = MCHOSE_SPEED_MIN;
        new_mode.speed_max      = MCHOSE_SPEED_MAX;
        new_mode.speed          = controller->GetModeSpeed(value);
    }

    if(has_color)
    {
        new_mode.flags         |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_RANDOM_COLOR;
        new_mode.colors_min     = 1;
        new_mode.colors_max     = 1;
        new_mode.colors.push_back(controller->GetModeColor(value));
        new_mode.color_mode     = controller->GetModeRandom(value) ? MODE_COLORS_RANDOM : MODE_COLORS_MODE_SPECIFIC;
    }

    modes.push_back(new_mode);
}

void RGBController_MCHOSEKeyboard::SetupZones()
{
    zone keyboard_zone;

    keyboard_zone.name          = "Keyboard";
    keyboard_zone.type          = ZONE_TYPE_MATRIX;
    keyboard_zone.leds_count    = sizeof(mchose_g75_pro_keys) / sizeof(mchose_g75_pro_keys[0]);
    keyboard_zone.leds_min      = keyboard_zone.leds_count;
    keyboard_zone.leds_max      = keyboard_zone.leds_count;

    unsigned int map[MCHOSE_G75_PRO_ROWS * MCHOSE_G75_PRO_COLS];

    for(unsigned int map_idx = 0; map_idx < MCHOSE_G75_PRO_ROWS * MCHOSE_G75_PRO_COLS; map_idx++)
    {
        map[map_idx] = 0xFFFFFFFF;
    }

    for(unsigned int led_idx = 0; led_idx < keyboard_zone.leds_count; led_idx++)
    {
        const mchose_key& key = mchose_g75_pro_keys[led_idx];

        map[key.row * MCHOSE_G75_PRO_COLS + key.col] = led_idx;

        led new_led;

        new_led.name            = key.name;
        new_led.value           = key.index;

        leds.push_back(new_led);
    }

    keyboard_zone.matrix_map.Set(MCHOSE_G75_PRO_ROWS, MCHOSE_G75_PRO_COLS, map);

    zones.push_back(keyboard_zone);

    SetupColors();
}

void RGBController_MCHOSEKeyboard::DeviceUpdateLEDs()
{
    RGBColor color_buf[MCHOSE_PER_KEY_LEDS];

    for(unsigned int led_idx = 0; led_idx < MCHOSE_PER_KEY_LEDS; led_idx++)
    {
        color_buf[led_idx] = 0;
    }

    for(unsigned int led_idx = 0; led_idx < leds.size(); led_idx++)
    {
        color_buf[leds[led_idx].value] = colors[led_idx];
    }

    controller->SetPerKeyColors(color_buf);
}

void RGBController_MCHOSEKeyboard::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_MCHOSEKeyboard::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_MCHOSEKeyboard::DeviceUpdateMode()
{
    const mode& current = modes[active_mode];

    if(current.value == MCHOSE_MODE_CUSTOM)
    {
        controller->SetMode(MCHOSE_MODE_CUSTOM, 0, 0, false);
        DeviceUpdateLEDs();
        return;
    }

    bool random = (current.color_mode == MODE_COLORS_RANDOM);

    if(current.color_mode == MODE_COLORS_MODE_SPECIFIC && current.colors.size() > 0)
    {
        controller->SetModeColor((unsigned char)current.value, current.colors[0]);
    }

    unsigned char brightness = (current.flags & MODE_FLAG_HAS_BRIGHTNESS) ? (unsigned char)current.brightness : MCHOSE_BRIGHTNESS_MAX;
    unsigned char speed      = (current.flags & MODE_FLAG_HAS_SPEED)      ? (unsigned char)current.speed      : MCHOSE_SPEED_DEFAULT;

    controller->SetMode((unsigned char)current.value, brightness, speed, random);
}
