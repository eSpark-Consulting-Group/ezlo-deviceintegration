#pragma once

#include <mutex>

namespace HubPlatform {
namespace threads {

template < typename T >
class UniqueSafeWrapper final {
public:
    UniqueSafeWrapper() = default;

    UniqueSafeWrapper( T&& value )
    : m_value( std::move( value ) ) {}

    UniqueSafeWrapper( const UniqueSafeWrapper& ) = delete;

    UniqueSafeWrapper& operator=( const UniqueSafeWrapper& ) = delete;

    UniqueSafeWrapper( UniqueSafeWrapper&& ) = default;

    UniqueSafeWrapper& operator=( UniqueSafeWrapper&& ) = default;

    template < typename FuncType >
    auto process( FuncType&& func ) const ->
        typename std::remove_cv< decltype( func( std::declval< const T& >() ) ) >::type {
        std::lock_guard< std::mutex > lock( m_mutex );
        return func( m_value );
    }

    template < typename FuncType >
    auto process( FuncType&& func ) -> typename std::remove_cv< decltype( func( std::declval< T& >() ) ) >::type {
        std::lock_guard< std::mutex > lock( m_mutex );
        return func( m_value );
    }

    template < typename ValueType = T >
    typename std::enable_if< std::is_copy_assignable< ValueType >::value, void >::type get( ValueType& value ) const {
        std::lock_guard< std::mutex > lock( m_mutex );
        value = m_value;
    }

    template < typename ValueType = T >
    typename std::enable_if< std::is_copy_assignable< ValueType >::value || std::is_move_assignable< ValueType >::value,
                             void >::type
    set( ValueType&& value ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        m_value = std::move( value );
    }

    template < typename ValueType = T >
    typename std::enable_if< std::is_copy_assignable< ValueType >::value, void >::type set( const ValueType& value ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        m_value = value;
    }

#if __cplusplus > 201103L
    template < typename ReturnType, typename... Args >
    void process( ReturnType ( T::*func )( Args&&... ), Args&&... args ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        ( m_value.*func )( std::forward< Args >( args )... );
    }
#endif

private:
    T m_value;
    mutable std::mutex m_mutex;
};

}  // namespace threads
}  // namespace HubPlatform
