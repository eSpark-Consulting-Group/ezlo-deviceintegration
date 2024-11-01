#pragma once

#include <cstring>

namespace TwoGig {

template < class T >
T fromBytes( const void* src ) noexcept {
    T dst;
    memcpy( &dst, src, sizeof( T ) );

    return dst;
}

}  // namespace TwoGig
