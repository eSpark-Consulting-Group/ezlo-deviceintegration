#pragma once

#include "2gig/pad_button.h"

#include <cstdint>

namespace TwoGig {

struct GenericDeviceStatus {
    bool keypad : 1;
    bool repeated : 1;
    bool supervisory : 1;
    bool lowBattery : 1;
    uint8_t : 4;
};

struct DoorWindowSensorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 1;
    bool reedClosed : 1;
    bool tamperDetected : 1;
    bool externalContact : 1;
};

struct SmokeDetectorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    bool freezeAlarm : 1;
    bool heatAlarm : 1;
    bool tamperDetected : 1;
    bool smokeAlarm : 1;
};

struct CoDetectorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 2;
    bool tamperDetected : 1;
    bool coAlarm : 1;
};

struct PirDetectorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 2;
    bool tamperDetected : 1;
    bool motionDetected : 1;
};

struct GlassBreakStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 2;
    bool tamperDetected : 1;
    bool glassBreakDetected : 1;
};

struct FloodTemperatureStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    bool freezeAlarm : 1;
    bool heatAlarm : 1;
    bool tamperDetected : 1;
    bool waterDetected : 1;
};

struct TiltDetectorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 2;
    bool tamperDetected : 1;
    bool tiltDetected : 1;
};

struct RemoteKeyFobStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    bool button1Pressed : 1;
    bool button2Pressed : 1;
    bool button3Pressed : 1;
    bool button4Pressed : 1;
};

struct PanicButtonStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 3;
    bool buttonPressed : 1;
};

struct PadStatus {
    bool keypad : 1;
    uint8_t : 1;
    bool supervisory : 1;
    bool lowBattery : 1;
    PadButton::Enum buttonPressed : 4;
};

struct RepeaterStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    uint8_t : 1;
    bool jamDetected : 1;
    bool tamperDetected : 1;
    bool acFailed : 1;
};

struct HoneywellSensorStatus {
    uint8_t : 3;
    bool lowBattery : 1;
    bool loop3 : 1;
    bool loop2 : 1;
    bool tamperDetected : 1;
    bool loop1 : 1;
};

struct ReachableStatus {
    bool isReachable : 1;
};

}  // namespace TwoGig
