#pragma once

#include <cstdint>

namespace TwoGig {

struct SerialPort {
    virtual ~SerialPort() = default;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool readByte( uint8_t& buffer ) const = 0;
};

}  // namespace TwoGig
