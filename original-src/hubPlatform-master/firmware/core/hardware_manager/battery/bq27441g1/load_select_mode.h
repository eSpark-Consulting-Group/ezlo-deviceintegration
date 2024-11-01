#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

struct LoadSelectModeValue {
    explicit LoadSelectModeValue( uint8_t bits ) {
        memcpy( this, &bits, 1 );
        verify();
    }

    uint8_t toUint8() const {
        uint16_t bits;
        memcpy( &bits, this, 1 );
        return bits;
    }

    bool verify() const {
        bool ok = ( RSVD01 == 0 ) && ( RSVD02 == 0 ) && ( RSVD03 == 0 ) && ( RSVD04 == 0 );
        assert( ok );
        return ok;
    }

    void fix() {
        RSVD01 = 0;
        RSVD02 = 0;
        RSVD03 = 0;
        RSVD04 = 0;
    }

    uint8_t LoadSelect : 3;  // 2:0
    uint8_t RSVD01 : 1;      // 3
    uint8_t RSVD02 : 1;      // 4
    uint8_t RSVD03 : 1;      // 5
    uint8_t RSVD04 : 1;      // 6
    uint8_t LoadMode : 1;    // 7
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
