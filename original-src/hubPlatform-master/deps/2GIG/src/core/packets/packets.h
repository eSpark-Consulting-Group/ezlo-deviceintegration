#pragma once

#include "2gig/device_type.h"
#include "2gig/device_status.h"
#include "utils/byte_cast.h"

#include <cstdint>
#include <vector>

namespace TwoGig {

using PacketBytes = std::vector< uint8_t >;

struct __attribute__( ( packed ) ) Transmission {
    uint8_t crc;
    int8_t signalStrength;
    uint32_t serial : 20;
    uint8_t deviceType : 4;
    uint8_t status;
    uint8_t packetId;
    uint8_t header;

public:
    DeviceType::Enum getDeviceType() const {
        if( getStatus< GenericDeviceStatus >().keypad )
            return DeviceType::Keypad;

        return static_cast< DeviceType::Enum >( deviceType );
    }

    template < typename StatusT >
    StatusT getStatus() const {
        return fromBytes< StatusT >( &status );
    }
};

struct Jamming {
    uint8_t crc;
    uint8_t : 8;
    uint8_t : 8;
    uint8_t : 8;
    uint8_t : 8;
    uint8_t : 8;
    uint8_t packetId;
    uint8_t header;
};

struct PowerOn {
    uint8_t crc;
    uint32_t serial : 24;
    uint8_t firmwareVersionMinor;
    uint8_t firmwareVersionMajor;
    uint8_t packetId;
    uint8_t header;
};

struct Supervisory {
    uint8_t header;
};

}  // namespace TwoGig
