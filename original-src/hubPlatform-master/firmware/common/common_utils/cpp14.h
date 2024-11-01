#pragma once

#if __cplusplus <= 201103L

#include <memory>
#include <type_traits>

namespace std {

// Note: this implementation does not disable this overload for array types
template < typename T, typename... Args >
std::unique_ptr< T > make_unique( Args&&... args ) {
    return std::unique_ptr< T >( new T( std::forward< Args >( args )... ) );
}

#if defined( __GNUG__ ) && ( __GNUC__ == 4 ) && ( __GNUC_MINOR__ == 8 )

template < typename T >
struct hash {
    using IsEnumCheck = typename std::enable_if< std::is_enum< T >::value, T >::type;

    size_t operator()( const T key ) const {
        using OriginalEnumType = typename std::underlying_type< T >::type;
        return hash< OriginalEnumType >()( static_cast< OriginalEnumType >( key ) );
    }
};

#endif

template < typename T >
using underlining_type_t = typename underlying_type< T >::type;

template < typename T >
using decay_t = typename decay< T >::type;

template < bool Condition, typename T = void >
using enable_if_t = typename enable_if< Condition, T >::type;

}  // namespace std
#endif
