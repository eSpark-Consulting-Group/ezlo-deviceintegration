#pragma once

#include "common_utils/cpp14.h"
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace common {

template < typename ElementType >
class PrioritizedTaskPool {
public:
    using DefaultTaskStartedCallback = std::function< void() >;

    PrioritizedTaskPool( DefaultTaskStartedCallback defaultTaskStartedCallback = []() {} )
    : m_defaultTaskStartedCallback( std::move( defaultTaskStartedCallback ) )
    , m_isRunning( true ) {
        m_thread = std::thread( &PrioritizedTaskPool< ElementType >::processTasks, this );
    }
    PrioritizedTaskPool( const PrioritizedTaskPool& ) = delete;
    PrioritizedTaskPool( PrioritizedTaskPool&& ) = delete;
    PrioritizedTaskPool& operator=( const PrioritizedTaskPool& ) = delete;
    PrioritizedTaskPool& operator=( PrioritizedTaskPool&& ) = delete;
    ~PrioritizedTaskPool() {
        shutdown();
    }

    void stopTask() {
        std::lock_guard< std::mutex > lock( m_taskQueueMutex );
        stop();
    }
    void run() {
        if( m_isRunning ) {
            return;
        }
        m_thread = std::thread( &PrioritizedTaskPool< ElementType >::processTasks, this );
    }
    void shutdown() {
        if( !m_isRunning ) {
            return;
        }
        std::unique_lock< std::mutex > lock( m_taskQueueMutex );
        m_isRunning = false;
        if( m_taskInWork ) {
            m_taskInWork->stop();
        }
        m_queueCondVar.notify_all();
        lock.unlock();
        if( m_thread.joinable() ) {
            m_thread.join();
        }
    }
    size_t queueSize() const {
        std::lock_guard< std::mutex > lock( m_taskQueueMutex );
        return m_taskQueue.size();
    }
    void setDefaultTask( ElementType&& element ) {
        std::unique_lock< std::mutex > lock( m_taskQueueMutex );
        if( m_defaultTask ) {
            m_defaultTask->stop();
        }
        m_defaultTask = std::make_unique< ElementType >( element );
        if( m_taskQueue.empty() ) {
            m_taskQueue.push( std::move( element ) );
            lock.unlock();
            m_queueCondVar.notify_one();
            m_defaultTaskStartedCallback();
        }
    }
    void submit( ElementType&& element ) {
        {
            std::lock_guard< std::mutex > lock( m_taskQueueMutex );
            m_taskQueue.push( std::move( element ) );
            if( m_taskInWork && ( *m_taskInWork > m_taskQueue.top() ) ) {
                stop();
            }
        }
        m_queueCondVar.notify_one();
    }
    bool safeSubmit( ElementType&& element ) {
        try {
            submit( std::move( element ) );
            return true;
        }
        catch( ... ) {
        }
        return false;
    }

public:
    using QueueType = std::priority_queue< ElementType, std::vector< ElementType >, std::greater< ElementType > >;

private:
    void processTasks() {
        std::unique_lock< std::mutex > lock( m_taskQueueMutex );
        while( m_isRunning ) {
            m_queueCondVar.wait( lock, [this] { return !( m_taskQueue.empty() && m_isRunning ); } );
            if( !m_taskQueue.empty() && m_isRunning ) {
                ElementType task = m_taskQueue.top();
                m_taskQueue.pop();
                m_taskInWork = std::make_unique< ElementType >( std::move( task ) );
                lock.unlock();
                ( *m_taskInWork )();
                lock.lock();
                m_taskInWork.reset();
                if( m_defaultTask && m_taskQueue.empty() ) {
                    m_taskQueue.push( *m_defaultTask );
                    lock.unlock();
                    m_defaultTaskStartedCallback();
                    lock.lock();
                }
            }
        }
    }
    void stop() {
        if( m_taskInWork ) {
            m_taskInWork->stop();
        }
    }

private:
    DefaultTaskStartedCallback m_defaultTaskStartedCallback;
    QueueType m_taskQueue;
    std::unique_ptr< ElementType > m_taskInWork;
    std::unique_ptr< ElementType > m_defaultTask;
    mutable std::mutex m_taskQueueMutex;
    std::condition_variable m_queueCondVar;
    std::thread m_thread;
    std::atomic_bool m_isRunning;
};

}  // namespace common
