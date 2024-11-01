#pragma once

#include "json_utils/json_fwd.h"

#include <cstdint>
#include <cstring>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

struct Flags {
    explicit Flags( uint16_t bits ) {
        memcpy( this, &bits, 2 );
    }

    uint16_t toUint16() const {
        uint16_t bits;
        memcpy( &bits, this, 2 );
        return bits;
    }

    Json toJson() const {
        Json flagsJson;
        flagsJson[ "DSG" ] = DSG;
        flagsJson[ "SOCF" ] = SOCF;
        flagsJson[ "SOC1" ] = SOC1;
        flagsJson[ "BAT_DET" ] = BAT_DET;
        flagsJson[ "CFGUPMODE" ] = CFGUPMODE;
        flagsJson[ "ITPOR" ] = ITPOR;
        flagsJson[ "OCVTAKEN" ] = OCVTAKEN;
        flagsJson[ "CHG" ] = CHG;
        flagsJson[ "FC" ] = FC;
        flagsJson[ "UT" ] = UT;
        flagsJson[ "OT" ] = OT;
        return flagsJson;
    }

    bool DSG : 1;        // 0
    bool SOCF : 1;       // 1
    bool SOC1 : 1;       // 2
    bool BAT_DET : 1;    // 3
    bool CFGUPMODE : 1;  // 4
    bool ITPOR : 1;      // 5
    bool RSVD1 : 1;      // 6
    bool OCVTAKEN : 1;   // 7
    bool CHG : 1;        // 8
    bool FC : 1;         // 9
    bool RSVD2 : 1;      // 10
    bool RSVD3 : 1;      // 11
    bool RSVD4 : 1;      // 12
    bool RSVD5 : 1;      // 13
    bool UT : 1;         // 14
    bool OT : 1;         // 15
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
