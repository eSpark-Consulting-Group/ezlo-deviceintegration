#pragma once

#include "log_utils/ezlo_log.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace LogReader {
namespace Container {

using Lock = std::mutex;
using UniqueLock = std::unique_lock< Lock >;
using LockGuard = std::lock_guard< Lock >;

class BlockingLogQueue {
public:
    BlockingLogQueue( const BlockingLogQueue& ) = delete;
    BlockingLogQueue& operator=( const BlockingLogQueue& ) = delete;
    BlockingLogQueue( BlockingLogQueue&& ) = delete;
    BlockingLogQueue& operator=( BlockingLogQueue&& ) = delete;

    BlockingLogQueue( const size_t consumerSize = 0 )
    : m_consumerSize( consumerSize )
    , m_wait( true )
    , m_waitEmpty( true ) {
        for( size_t index = 0; index < m_consumerSize; index++ ) {
            m_consumerIndexes[ index ] = 0;
        }
    }

    void push( const std::string& value ) {
        pushImpl( value );
    }

    void push( const std::string&& value ) {
        pushImpl( std::move( value ) );
    }

    bool pop( const size_t consumerIndex, std::string& value ) try {
        if( m_consumerSize != 0 && m_consumerSize <= consumerIndex ) {
            return false;
        }

        {
            LockGuard lock( m_queueMutex );
            if( m_queue.empty() ) {
                m_emptyCondition.notify_all();
                if( !m_wait ) {
                    return false;
                }
            }
        }

        {
            UniqueLock lock( m_queueMutex );
            bool findItem = false;
            m_condition.wait( lock, [&] {
                if( !m_queue.empty() &&
                    ( m_consumerIndexes.empty() || ( m_consumerIndexes[ consumerIndex ] < m_queue.size() ) ) ) {
                    findItem = true;
                    return true;
                }

                return !m_wait;
            } );

            if( m_queue.empty() || !findItem ) {
                if( m_queue.empty() ) {
                    m_emptyCondition.notify_all();
                }
                return false;
            }
        }

        {
            LockGuard lock( m_queueMutex );
            bool result = false;
            size_t index = 0;
            bool eraseState = true;
            if( !m_consumerIndexes.empty() ) {
                index = m_consumerIndexes[ consumerIndex ];
                if( index >= m_queue.size() ) {
                    m_consumerIndexes[ consumerIndex ] = m_queue.size();
                }
                else {
                    m_consumerIndexes[ consumerIndex ]++;
                    for( const auto& it : m_consumerIndexes ) {
                        if( it.second <= index ) {
                            eraseState = false;
                            break;
                        }
                    }
                }
            }

            if( index < m_queue.size() ) {
                value = m_queue[ index ];
                if( eraseState ) {
                    popFront();
                }
                result = true;
            }

            if( m_queue.empty() ) {
                m_emptyCondition.notify_all();
            }

            return result;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Error caught while pop log message:" + std::string( ex.what() ) ) );
        return false;
    }

    void unblockConsumer() {
        {
            LockGuard lock( m_queueMutex );
            m_wait = false;
        }
        m_condition.notify_all();
    }

    void reset() {
        LockGuard lock( m_queueMutex );
        m_wait = true;
    }

    void clear() {
        UniqueLock lock( m_queueMutex );

        if( !m_queue.empty() ) {
            std::deque< std::string > tmp;
            tmp.swap( m_queue );
        }
    }

    void unblockWaitUntilQueueIsEmpty() {
        {
            LockGuard lock( m_queueEmptyMutex );
            m_waitEmpty = false;
        }
        m_emptyCondition.notify_all();
    }

    void waitUntilQueueIsEmpty() {
        {
            UniqueLock lock( m_queueEmptyMutex );
            m_emptyCondition.wait( lock, [&] { return m_queue.empty() || !m_waitEmpty; } );
        }
        {
            LockGuard lock( m_queueEmptyMutex );
            m_waitEmpty = true;
        }
    }

    size_t size() const {
        UniqueLock lock( m_queueMutex );
        return m_queue.size();
    }

protected:
    void popFront() {
        m_queue.pop_front();
        if( m_consumerIndexes.empty() ) {
            return;
        }
        for( auto& it : m_consumerIndexes ) {
            if( it.second > 0 ) {
                it.second--;
            }
        }
    }

    virtual void pushImpl( const std::string& value ) {
        {
            UniqueLock lock( m_queueMutex );
            m_queue.emplace_back( value );
        }
        m_condition.notify_all();
    }

public:
    const size_t m_consumerSize;

protected:
    std::map< size_t, size_t > m_consumerIndexes;
    std::deque< std::string > m_queue;
    std::atomic_bool m_wait;
    std::atomic_bool m_waitEmpty;
    mutable Lock m_queueMutex;
    mutable Lock m_queueEmptyMutex;
    std::condition_variable m_condition;
    std::condition_variable m_emptyCondition;
};

}  // namespace Container
}  // namespace LogReader
}  // namespace HubPlatform
