#pragma once

namespace TwoGig {

struct DeviceType {
    enum Enum {
        SmokeDetector = 0x2,
        PanicButton = 0x3,
        PirDetector = 0x4,
        FloodTemperature = 0x7,
        HoneywellSensor = 0x8,
        GlassBreakDetector = 0x9,
        DoorWindowSensor = 0xA,
        CoDetector = 0xB,
        TiltDetector = 0xC,
        RemoteKeyFob = 0xF,
        SmokeCoDetector = 0xFC,           // Note: has no corresponding value (you must not cast this enum to/from int)
        DoorWindowWithWireSensor = 0xFD,  // Note: has no corresponding value (you must not cast this enum to/from int)
        Repeater = 0xFE,                  // Note: has no corresponding value (you must not cast this enum to/from int)
        Keypad = 0xFF                     // Note: has no corresponding value (you must not cast this enum to/from int)
    };
};

}  // namespace TwoGig
