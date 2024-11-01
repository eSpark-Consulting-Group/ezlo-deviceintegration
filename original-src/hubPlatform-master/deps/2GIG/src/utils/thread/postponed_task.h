#pragma once

#include "2gig/pin_code_state.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <chrono>

namespace TwoGig {

struct PostponedTask {
    PostponedTask( std::function< void() > onTimeout )
    : m_onTimeout( onTimeout ) {}

    ~PostponedTask() {
        stop();
    }

    template < typename DurationT >
    void reschedule( const DurationT& timeout ) {
        std::promise< void > threadStarted;

        m_timeoutJob = std::thread( [this, &threadStarted, &timeout] {
            std::unique_lock< std::mutex > lock( m_mutex );
            threadStarted.set_value();
            if( m_condition.wait_for( lock, timeout ) == std::cv_status::timeout ) {
                m_onTimeout();
            }
        } );

        // Wait until the mutex is locked in the thread so that we don't block with the next line
        threadStarted.get_future().wait();

        // Wait until the condition variable is waited upon (the mutex is atomically unlocked at the same time)
        std::lock_guard< std::mutex > lock( m_mutex );
    }

    void stop() {
        m_condition.notify_one();

        if( m_timeoutJob.joinable() )
            m_timeoutJob.join();
    }

private:
    std::function< void() > m_onTimeout;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_timeoutJob;
};

}  // namespace TwoGig
