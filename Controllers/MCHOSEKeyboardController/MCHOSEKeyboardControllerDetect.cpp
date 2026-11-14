/*---------------------------------------------------------*\
| MCHOSEKeyboardControllerDetect.cpp                        |
|                                                           |
|   Detector for MCHOSE keyboards                           |
|                                                           |
|   Sourav Jagtap                               14 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <hidapi.h>
#include "DetectionManager.h"
#include "MCHOSEKeyboardController.h"
#include "RGBController_MCHOSEKeyboard.h"

/*---------------------------------------------------------*\
| MCHOSE (Compx) vendor ID                                  |
\*---------------------------------------------------------*/
#define MCHOSE_VID                                  0x41E4

/*---------------------------------------------------------*\
| MCHOSE product IDs                                        |
|   Over USB the keyboard enumerates as 258A:010C and is    |
|   detected by DetectSinowealthKeyboard10c, which hands it |
|   to this driver by model ID                              |
\*---------------------------------------------------------*/
#define MCHOSE_2_4G_RECEIVER_PID                    0x2001

DetectedControllers DetectMCHOSEKeyboard(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev;

    dev = hid_open_path(info->path);

    if(dev)
    {
        MCHOSEKeyboardController*     controller     = new MCHOSEKeyboardController(dev, info->path, name, false);
        RGBController_MCHOSEKeyboard* rgb_controller = new RGBController_MCHOSEKeyboard(controller);

        detected_controllers.push_back(rgb_controller);
    }

    return(detected_controllers);
}

REGISTER_HID_DETECTOR_IPU("MCHOSE G75 Pro (2.4G)", DetectMCHOSEKeyboard, MCHOSE_VID, MCHOSE_2_4G_RECEIVER_PID, 1, 0xFF02, 0x02);
