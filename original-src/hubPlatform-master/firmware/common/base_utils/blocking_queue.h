#pragma once

#include <condition_variable>
#include <queue>
#include <atomic>

namespace HubPlatform {
namespace Container {
template < typename T >
class BlockingQueue {
public:
    BlockingQueue()
    : m_wait( true ) {}

    void push( const T& value ) {
        push_iml( value );
    }

    void push( const T&& value ) {
        push_iml( std::move( value ) );
    }

    bool pop( T& value ) {
        std::unique_lock< std::mutex > lock( m_queueMutex );
        m_condition.wait( lock, [this] { return !( m_wait && m_queue.empty() ); } );
        if( m_queue.empty() )
            return false;
        value = m_queue.front();
        m_queue.pop();
        return true;
    }

    void unblockConsumer() {
        m_wait = false;
        m_condition.notify_one();
    }

    void reset() {
        m_wait = true;
    }

    void clear() {
        std::unique_lock< std::mutex > lock( m_queueMutex );

        if( !m_queue.empty() ) {
            std::queue< T > tmp;
            tmp.swap( m_queue );
        }
    }

    size_t size() const {
        std::unique_lock< std::mutex > lock( m_queueMutex );
        return m_queue.size();
    }

protected:
    virtual void push_iml( const T& value ) {
        {
            std::unique_lock< std::mutex > lock( m_queueMutex );
            m_queue.emplace( value );
        }

        m_condition.notify_one();
    }

protected:
    std::atomic_bool m_wait;
    mutable std::mutex m_queueMutex;
    std::queue< T > m_queue;
    std::condition_variable m_condition;
};
}  // namespace Container

}  // namespace HubPlatform
