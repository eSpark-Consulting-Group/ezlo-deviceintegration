#pragma once

#include <mutex>
#include <atomic>
#include <utility>

namespace common {
namespace utils {

template < class T >
class EzloSingleton {
public:
    EzloSingleton() = delete;
    EzloSingleton( const EzloSingleton& singleton ) = delete;
    EzloSingleton& operator=( const EzloSingleton& singleton ) = delete;

    static T* instance();
    template < class... Args >
    static T* instance( Args&&... args );

private:
    ~EzloSingleton();

private:
    static std::atomic< T* > p_instance_;
    static std::mutex mutex_;
};

template < class T >
std::atomic< T* > EzloSingleton< T >::p_instance_ = { nullptr };

template < class T >
std::mutex EzloSingleton< T >::mutex_;

template < class T >
EzloSingleton< T >::~EzloSingleton() {
    if( p_instance_ != nullptr ) {
        delete p_instance_;
    }
}

template < class T >
T* EzloSingleton< T >::instance() {
    if( p_instance_ == nullptr ) {
        std::lock_guard< std::mutex > lock( mutex_ );
        if( p_instance_ == nullptr ) {
            p_instance_ = new T();
        }
    }
    return p_instance_;
}

template < class T >
template < class... Args >
T* EzloSingleton< T >::instance( Args&&... args ) {
    if( p_instance_ == nullptr ) {
        std::lock_guard< std::mutex > lock( mutex_ );
        if( p_instance_ == nullptr ) {
            p_instance_ = new T( std::forward< Args >( args )... );
        }
    }
    return p_instance_;
}
}  // namespace utils
}  // namespace common