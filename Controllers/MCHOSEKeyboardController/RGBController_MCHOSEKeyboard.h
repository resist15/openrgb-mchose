/*---------------------------------------------------------*\
| RGBController_MCHOSEKeyboard.h                            |
|                                                           |
|   RGBController for MCHOSE keyboards                      |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <functional>
#include "MCHOSEKeyboardController.h"
#include "RGBController.h"

class RGBController_MCHOSEKeyboard : public RGBController
{
public:
    RGBController_MCHOSEKeyboard(MCHOSEKeyboardController* controller_ptr, std::function<void()> destroy_callback = nullptr);
    ~RGBController_MCHOSEKeyboard();

    void        SetupZones();

    void        DeviceUpdateLEDs();
    void        DeviceUpdateZoneLEDs(int zone);
    void        DeviceUpdateSingleLED(int led);

    void        DeviceUpdateMode();

private:
    MCHOSEKeyboardController*   controller;
    std::function<void()>       on_destroy;

    void        AddEffectMode(const char* mode_name, unsigned char value, bool has_brightness, bool has_speed, bool has_color);
};
