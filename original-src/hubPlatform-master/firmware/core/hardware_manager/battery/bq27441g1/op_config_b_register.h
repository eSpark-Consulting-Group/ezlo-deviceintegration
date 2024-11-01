#pragma once

#include "json_utils/json_fwd.h"

#include <cstdint>
#include <cstring>
#include <cassert>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

struct OpConfigBRegister {
    explicit OpConfigBRegister( uint8_t bits ) {
        memcpy( this, &bits, 1 );
        verify();
    }

    uint8_t toUint8() const {
        uint16_t bits;
        memcpy( &bits, this, 1 );
        return bits;
    }

    Json toJson() const {
        Json opConfigBJson;
        opConfigBJson[ "SMOOTHEN" ] = SMOOTHEN;
        return opConfigBJson;
    }

    bool verify() const {
        bool ok = ( RSVD01 == 0 ) && ( RSVD02 == 0 ) && ( RSVD03 == 0 ) && ( RSVD04 == 0 ) && ( RSVD11 == 1 ) &&
                  ( RSVD12 == 1 ) && ( RSVD13 == 1 );
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
    }

    uint8_t RSVD11 : 1;  // 0
    uint8_t RSVD12 : 1;  // 1
    bool SMOOTHEN : 1;   // 2
    uint8_t RSVD13 : 1;  // 3
    uint8_t RSVD01 : 1;  // 4
    uint8_t RSVD02 : 1;  // 5
    uint8_t RSVD03 : 1;  // 6
    uint8_t RSVD04 : 1;  // 7
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
