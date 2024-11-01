#pragma once

#include <cstdint>
#include <cstddef>

namespace TwoGig {

struct PersistenceClient {
    virtual ~PersistenceClient() = default;

    virtual void writeBytes( const uint8_t* bytes, size_t count ) = 0;
    virtual void readBytes( uint8_t* buffer, size_t count ) = 0;
};

}  // namespace TwoGig
