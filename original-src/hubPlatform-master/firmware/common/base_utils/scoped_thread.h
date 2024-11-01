#pragma once
#include <iostream>
#include <thread>
#include <stdexcept>

namespace common {
class ScopedThread {
public:
    ScopedThread() {}
    explicit ScopedThread( std::thread tr )
    : m_thread( std::move( tr ) ) {
        if( !m_thread.joinable() ) {
            std::cerr << "Warning! Thread is not running! ScopedThread" << std::endl;
        }
    }

    ~ScopedThread() {
        if( m_thread.joinable() )
            m_thread.join();
        else
            std::cerr << "Warning! Thread joining failed! ~ScopedThread" << std::endl;
    }

    ScopedThread& operator=( std::thread&& thread ) noexcept {
        m_thread = std::move( thread );
        return *this;
    }

public:
    ScopedThread( const ScopedThread& ) = delete;
    ScopedThread& operator=( const ScopedThread& ) = delete;

private:
    std::thread m_thread;
};
}  // namespace common
