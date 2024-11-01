#pragma once

#include <mutex>
#include <functional>

namespace TwoGig {

template < typename T >
struct ThreadSafeWrapper {
private:
    T m_obj;
    mutable std::mutex m_mutex;

public:
    template < typename... ArgsT >
    explicit ThreadSafeWrapper( ArgsT&&... args )
    : m_obj( std::forward< ArgsT >( args )... ) {}

    template < typename Fn >
    auto with( Fn action ) -> decltype( action( m_obj ) ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        return action( m_obj );
    }

    template < typename Fn >
    auto with( Fn action ) const -> decltype( action( m_obj ) ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        return action( m_obj );
    }

private:
    ThreadSafeWrapper( const ThreadSafeWrapper& ) = delete;
    ThreadSafeWrapper& operator=( const ThreadSafeWrapper& ) = delete;
};

}  // namespace TwoGig
