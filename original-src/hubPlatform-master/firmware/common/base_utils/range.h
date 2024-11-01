#pragma once

#include <algorithm>
#include <functional>

enum class RangeType { Included, Excluded };

template < typename T >
struct Range {
    Range( const T& min, const T& max, RangeType type )
    : min( min )
    , max( max )
    , type( type ) {}
    Range( T&& min, T&& max, RangeType type )
    : min( std::move( min ) )
    , max( std::move( max ) )
    , type( type ) {}

    Range( const Range< T >& ) = default;
    Range( Range< T >&& ) = default;
    Range< T >& operator=( const Range< T >& ) = default;
    Range< T >& operator=( Range< T >&& ) = default;

    bool isIntersection( const Range< T >& other ) const {
        if( type == RangeType::Excluded && other.type == RangeType::Excluded ) {
            return true;
        }
        else if( type == RangeType::Excluded && other.type == RangeType::Included ) {
            return min > other.min || other.max > max;
        }
        else if( type == RangeType::Included && other.type == RangeType::Excluded ) {
            return min < other.min || other.max < max;
        }
        else {
            return !( min > other.max || other.min > max );
        }
    }

    bool operator==( const Range< T >& other ) const {
        return min == other.min && max == other.max && type == other.type;
    }

    T min;
    T max;
    RangeType type;
};
