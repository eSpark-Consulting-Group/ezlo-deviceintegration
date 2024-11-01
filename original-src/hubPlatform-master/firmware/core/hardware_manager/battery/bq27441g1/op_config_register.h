#pragma once

#include "json_utils/json_fwd.h"

#include <cstdint>
#include <cstring>
#include <cassert>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

struct OpConfigRegister {
    explicit OpConfigRegister( uint16_t bits ) {
        memcpy( this, &bits, 2 );
        verify();
    }

    uint16_t toUint16() const {
        uint16_t bits;
        memcpy( &bits, this, 2 );
        return bits;
    }

    Json toJson() const {
        Json opConfigJson;
        opConfigJson[ "TEMPS" ] = TEMPS;
        opConfigJson[ "BATLOWEN" ] = BATLOWEN;
        opConfigJson[ "RMFCC" ] = RMFCC;
        opConfigJson[ "SLEEP" ] = SLEEP;
        opConfigJson[ "GPIOPOL" ] = GPIOPOL;
        opConfigJson[ "BI_PU_EN" ] = BI_PU_EN;
        opConfigJson[ "BIE" ] = BIE;
        return opConfigJson;
    }

    bool verify() const {
        bool ok = ( RSVD01 == 0 ) && ( RSVD02 == 0 ) && ( RSVD03 == 0 ) && ( RSVD04 == 0 ) && ( RSVD11 == 1 ) &&
                  ( RSVD12 == 1 ) && ( RSVD13 == 1 ) && ( RSVD14 == 1 ) && ( RSVD15 == 1 );
        assert( ok );
        return ok;
    }

    void fix() {
        RSVD01 = 0;
        RSVD02 = 0;
        RSVD03 = 0;
        RSVD04 = 0;

        RSVD11 = 1;
        RSVD12 = 1;
        RSVD13 = 1;
        RSVD14 = 1;
        RSVD15 = 1;
    }

    bool TEMPS : 1;      // 0
    uint8_t RSVD01 : 1;  // 1
    bool BATLOWEN : 1;   // 2
    uint8_t RSVD11 : 1;  // 3
    bool RMFCC : 1;      // 4
    bool SLEEP : 1;      // 5
    uint8_t RSVD12 : 1;  // 6
    uint8_t RSVD13 : 1;  // 7
    uint8_t RSVD14 : 1;  // 8
    uint8_t RSVD02 : 1;  // 9
    uint8_t RSVD15 : 1;  // 10
    bool GPIOPOL : 1;    // 11
    bool BI_PU_EN : 1;   // 12
    bool BIE : 1;        // 13
    uint8_t RSVD03 : 1;  // 14
    uint8_t RSVD04 : 1;  // 15
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
