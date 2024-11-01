#pragma once

#include <cmath>
#include <limits>
#include <type_traits>

namespace HubPlatform {

template < typename Float >
using EnableForFloat = typename std::enable_if< std::is_floating_point< Float >::value >::type;

template < typename Float, EnableForFloat< Float >* = nullptr >
inline bool areFloatsEqual( Float lhs, Float rhs ) noexcept {
    return std::abs( lhs - rhs ) <= std::numeric_limits< Float >::epsilon();
}

template < typename Float, EnableForFloat< Float >* = nullptr >
inline bool isFloatLess( Float lhs, Float rhs ) noexcept {
    return rhs - lhs > std::numeric_limits< Float >::epsilon();
}

}  // namespace HubPlatform
