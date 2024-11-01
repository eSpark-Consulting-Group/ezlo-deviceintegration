#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace TwoGig {

template < typename T >
struct BlockingQueue {
    void push( const T& value ) {
        {
            std::unique_lock< std::mutex > lock( m_mutex );
            m_queue.push( value );
        }

        unblockConsumer();
    }

    bool pop( T& value ) {
        std::unique_lock< std::mutex > lock( m_mutex );

        if( m_queue.empty() ) {
            m_condition.wait( lock );

            // Note: may be notified from outside, in which case the queue may still be empty
            if( m_queue.empty() )
                return false;
        }

        value = m_queue.front();
        m_queue.pop();

        return true;
    }

    void unblockConsumer() {
        m_condition.notify_one();
    }

    void clear() {
        std::unique_lock< std::mutex > lock( m_mutex );

        while( !m_queue.empty() )
            m_queue.pop();
    }

private:
    std::queue< T > m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

}  // namespace TwoGig
