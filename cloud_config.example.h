#pragma once

// ============================================================================
//  VOICE CONTROL  -  Arduino Cloud (free) -> Google Home -> Gemini / Assistant
// ============================================================================
//
//  Full walkthrough: docs/VOICE-CONTROL.md
//
//  Quick version:
//  1. Create a free account at https://cloud.arduino.cc/
//  2. Create a Thing called "Smart Home" with FOUR variables:
//         relay1, relay2, relay3, relay4
//     Variable type: Smart home -> Switch   (CloudSwitch)
//     Permission   : Read & Write
//     Update       : On Change
//     Enable "Smart Home Integration" (Google Home) for each variable.
//  3. Associate an ESP32 device with that Thing.
//  4. Copy the Board ID and Secret Device Key from the device page below.
//  5. Flash, connect Wi-Fi, then link the Arduino action in the Google Home app.
//
//  HOW TO USE THIS FILE
//  --------------------
//  Copy this file to "cloud_config.h" (same folder) and fill in your values.
//  "cloud_config.h" is git-ignored, so your credentials never end up in a
//  commit. Leave both strings empty to compile with the local dashboard only -
//  everything local (dashboard, switches, schedules) works without the cloud.
// ============================================================================

#define ENABLE_ARDUINO_CLOUD 1

#define CLOUD_BOARD_ID   ""
#define CLOUD_DEVICE_KEY ""

// Optional first-boot Wi-Fi credentials. Normally you will enter Wi-Fi in the
// captive portal instead, so these can stay empty.
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASS ""
