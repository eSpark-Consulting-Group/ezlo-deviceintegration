#pragma once

#include "json_utils/json_fwd.h"

#include <cstdint>
#include <cstring>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

struct ControlStatus {
    explicit ControlStatus( uint16_t bits ) {
        memcpy( this, &bits, 2 );
    }

    uint16_t toUint16() const {
        uint16_t bits;
        memcpy( &bits, this, 2 );
        return bits;
    }

    Json toJson() const {
        Json statusJson;
        statusJson[ "VOK" ] = VOK;
        statusJson[ "RUP_DIS" ] = RUP_DIS;
        statusJson[ "LDMD" ] = LDMD;
        statusJson[ "HIBERNATE" ] = HIBERNATE;
        statusJson[ "INITCOMP" ] = INITCOMP;
        statusJson[ "RES_UP" ] = RES_UP;
        statusJson[ "QMAX_UP" ] = QMAX_UP;
        statusJson[ "BCA" ] = BCA;
        statusJson[ "CCA" ] = CCA;
        statusJson[ "CALMODE" ] = CALMODE;
        statusJson[ "SS" ] = SS;
        statusJson[ "WDRESET" ] = WDRESET;
        statusJson[ "SHUTDOWNEN" ] = SHUTDOWNEN;
        return statusJson;
    }

    bool : 1;             // 0
    bool VOK : 1;         // 1
    bool RUP_DIS : 1;     // 2
    bool LDMD : 1;        // 3
    bool SLEEP : 1;       // 4
    bool : 1;             // 5
    bool HIBERNATE : 1;   // 6
    bool INITCOMP : 1;    // 7
    bool RES_UP : 1;      // 8
    bool QMAX_UP : 1;     // 9
    bool BCA : 1;         // 10
    bool CCA : 1;         // 11
    bool CALMODE : 1;     // 12
    bool SS : 1;          // 13
    bool WDRESET : 1;     // 14
    bool SHUTDOWNEN : 1;  // 15
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
