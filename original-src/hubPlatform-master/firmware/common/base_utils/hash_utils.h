#pragma once

#include <functional>

namespace common {

inline std::size_t hash_combine( std::size_t lhs, std::size_t rhs ) {
    return lhs ^= rhs + 0x9e3779b9 + ( lhs << 6 ) + ( lhs >> 2 );
}

template < typename T >
std::size_t hash_value( const T& value ) {
    return std::hash< T >()( value );
}

}  // namespace common
