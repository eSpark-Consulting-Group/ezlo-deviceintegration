#pragma once

#if __cplusplus < 201402L

#include <memory>

namespace std {

template < typename T, typename... ArgsT >
std::unique_ptr< T > make_unique( ArgsT&&... args ) {
    return std::unique_ptr< T >( new T{ std::forward< ArgsT >( args )... } );
}

}  // namespace std

#endif  // __cplusplus < 201402L