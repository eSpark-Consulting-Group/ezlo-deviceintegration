#pragma once

#include <cassert>

namespace TwoGig {

struct PacketId {
    enum Enum {
        TwoGig8ByteTransmission = 0xAE,
        HoneywellTransmission = 0xAF,
        TwoGig11ByteTransmission = 0xB0,
        JamDetect = 0x57,
        JamRestore = 0x75,
        PowerOn = 0x1F
    };

    static const char* toString( Enum value ) {
        switch( value ) {
        case TwoGig8ByteTransmission:
            return "TwoGig8ByteTransmission";

        case HoneywellTransmission:
            return "HoneywellTransmission";

        case TwoGig11ByteTransmission:
            return "TwoGig11ByteTransmission";

        case JamDetect:
            return "JamDetect";

        case JamRestore:
            return "JamRestore";

        case PowerOn:
            return "PowerOn";

        default:
            assert( 0 );
            return "Unknown";
        }
    }
};

}  // namespace TwoGig
