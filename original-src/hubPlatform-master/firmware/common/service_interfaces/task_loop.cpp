#include "service_interfaces/task_loop.h"
#include "service_interfaces/strand.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <cassert>
#include <future>

namespace HubPlatform {
namespace Services {

namespace {

constexpr const size_t InvalidHeapIndex = std::numeric_limits< size_t >::max();
using PostponedTaskFunctionPtr = std::shared_ptr< PostponedTaskFunction >;

template < typename Heap, typename NextIndexGetter >
void heapify( Heap& heap, size_t index, NextIndexGetter&& nextIndexGetter ) {
    assert( index < heap.size() );
    size_t nextIndex = index;
    if( nextIndexGetter( heap[ index ], nextIndex ) ) {
        auto source = std::move( heap[ index ] );
        do {
            heap[ index ] = std::move( heap[ nextIndex ] );
            index = nextIndex;
        } while( nextIndexGetter( source, nextIndex ) );

        heap[ index ] = std::move( source );
    }
}

template < typename Heap >
void heapifyDown( Heap& heap, size_t index ) {
    heapify( heap, index, [&heap]( const typename Heap::value_type& source, size_t& index ) {
        if( index == 0 ) {
            return false;
        }

        index = ( index - 1 ) / 2;
        return source < heap[ index ];
    } );
}

template < typename Heap >
void heapifyUp( Heap& heap, size_t index ) {
    heapify( heap, index, [&heap]( const typename Heap::value_type& source, size_t& index ) {
        const auto left = index * 2u + 1;
        if( left >= heap.size() ) {
            return false;
        }

        const auto right = left + 1u;
        index = right < heap.size() && heap[ right ] < heap[ left ] ? right : left;
        return heap[ index ] < source;
    } );
}

}  // namespace

class TaskLoop::TimeProviderImpl : public TaskLoop::TimeProvider {
public:
    TimePoint now() const override {
        return std::chrono::steady_clock::now();
    }
    std::cv_status waitUntil( std::unique_lock< std::mutex >& locker,
                              std::condition_variable& condVar,
                              const TimePoint& timePoint ) override {
        return condVar.wait_until( locker, timePoint );
    }
};

class TaskLoop::PostponedTaskImpl : public PostponedTask, public std::enable_shared_from_this< PostponedTaskImpl > {
public:
    class CallbackRunner {
    public:
        CallbackRunner( PostponedTaskImplPtr& taskPtr, std::unique_lock< std::mutex >& locker )
        : m_taskPtr( taskPtr )
        , m_locker( locker ) {
            assert( m_taskPtr );
            m_key = m_taskPtr->getKey();
            ++m_taskPtr->m_callCount;
            auto& loop = m_taskPtr->m_loop;
            const auto shouldNotify = !loop.m_tasksOnWaiting.empty() && !loop.m_tasks.empty() &&
                                      loop.m_unlimitedWaitingsCount == 0 &&
                                      loop.m_tasks.front().m_task->getKey() < loop.m_tasksOnWaiting.begin()->first;

            m_locker.unlock();
            // Wake up some thread to handle front task;
            // If m_unlimitedWaitingsCount != 0 it's no reason to wake up a thread because m_unlimitedWaitingCv must be
            // already notified
            if( shouldNotify ) {
                loop.m_timeoutWaitingCv.notify_one();
            }
        }

        void operator()() const {
            auto& taskFunctionPtr = m_taskPtr->m_taskFunctionPtr;
            if( taskFunctionPtr ) {
                assert( *taskFunctionPtr );
                ( *taskFunctionPtr )( *m_taskPtr );
            }
        }

        /** TODO: EFWR-4809: Freeze of task loop happens here.
         * Maybe, different nature of locks causes it?
         * m_locker - wraps std::mutex
         * m_taskPtr can have pointer to retain_release_ptr, which uses pthread recursive mutex.
         */
        ~CallbackRunner() {
            m_locker.lock();
            auto callCount = --m_taskPtr->m_callCount;
            auto waitingCvPtr = m_taskPtr->m_waitingCv;

            if( !m_taskPtr->isPeriodic() ) {
                if( m_taskPtr->getKey().second == m_key.second )
                    m_taskPtr->disable();

                // NOTE: EFWR-3600 Task lambda can store captured WS object
                // that locks global mutex (g_pRefMutex) on destruction.
                // Since WS global mutex and task loop mutex can be locked by
                // both WS and task loop threads, this can lead to deadlocks.
                // NOTE: Resolved in EFWR-4852: No global mutex in smart pointers called, traits used to replace the
                // mutex with specific one not related to websockets communications.
                m_locker.unlock();
                m_taskPtr.reset();
                m_locker.lock();
            }

            if( callCount == 0 ) {
                if( auto waitingCv = waitingCvPtr.lock() ) {
                    waitingCv->notify_all();
                }
            }
        }

    private:
        PostponedTaskImplPtr& m_taskPtr;
        PostponedTaskKey m_key;
        std::unique_lock< std::mutex >& m_locker;
    };

    PostponedTaskImpl( TaskLoop& loop, PostponedTaskFunction taskFunction )
    : m_loop( loop )
    , m_taskFunctionPtr( createFunctionPtr( std::move( taskFunction ) ) ) {}

    bool isActive() const override {
        return m_isActive;
    }

    void start( const Interval& interval, bool isPeriodic ) override {
        startImpl( interval, interval, isPeriodic );
    }

    void start( const Interval& first, const Interval& periodic ) override {
        startImpl( first, periodic, true );
    }

    void setCallback( PostponedTaskFunction taskFunction ) override {
        auto taskFunctionPtr = createFunctionPtr( std::move( taskFunction ) );

        std::unique_lock< std::mutex > locker( m_loop.m_mutex );
        auto oldTask = std::move( m_taskFunctionPtr );  // NOTE: EFWR-3600 Must be destructed not under lock
        m_taskFunctionPtr = std::move( taskFunctionPtr );
        locker.unlock();
    }

    void stop( StoppingMode mode ) override {
        if( mode == StoppingMode::OnTerminationSignalReceived ) {
            m_loop.stop( mode );
            return;
        }

        std::unique_lock< std::mutex > locker( m_loop.m_mutex );
        if( isActive() ) {
            removeFromWaitingsWithTimeout();
            disable();

            if( m_heapIndex != InvalidHeapIndex ) {
                if( m_heapIndex + 1 != m_loop.m_tasks.size() ) {
                    assert( m_loop.m_tasks[ m_heapIndex ].m_task == shared_from_this() );
                    m_loop.m_tasks[ m_heapIndex ] = std::move( m_loop.m_tasks.back() );

                    m_loop.m_tasks.pop_back();
                    m_loop.m_tasks[ m_heapIndex ].m_task->onKeyChanged( m_key );
                }
                else {
                    m_loop.m_tasks.pop_back();
                }

                m_heapIndex = InvalidHeapIndex;
            }
        }

        if( mode == StoppingMode::WaitingIfPossible && m_callCount &&
            m_loop.m_activeThreads.find( std::this_thread::get_id() ) == m_loop.m_activeThreads.end() ) {
            auto cv = m_waitingCv.lock();
            if( !cv ) {
                cv = std::make_shared< std::condition_variable >();
                m_waitingCv = cv;
            }

            cv->wait( locker, [this]() { return m_callCount == 0 || m_isActive; } );
        }
    }

    bool isPeriodic() const override {
        return m_isPeriodic;
    }
    Interval getInterval() const override {
        return m_period;
    }

    void setPeriodicMode( PeriodicMode mode ) override {
        m_periodicMode = mode;
    }

    PeriodicMode getPeriodicMode() const {
        return m_periodicMode;
    }

    void disable() {
        m_key = {};
        m_isActive = false;
    }

    const PostponedTaskKey& getKey() const {
        return m_key;
    }

    void setKey( const PostponedTaskKey& key ) {
        m_key = key;
    }

    size_t getHeapIndex() const {
        return m_heapIndex;
    }

    void setHeapIndex( size_t index ) {
        m_heapIndex = index;
    }

    void addToHeap() {
        assert( m_heapIndex == InvalidHeapIndex );
        m_heapIndex = m_loop.m_tasks.size();
        m_loop.m_tasks.push_back( { shared_from_this() } );
        heapifyDown( m_loop.m_tasks, m_heapIndex );
    }

    Interval getTimeToExecution() override {
        return getTimeToExecutionImpl();
    }

private:
    static PostponedTaskFunctionPtr createFunctionPtr( PostponedTaskFunction taskFunction ) {
        return taskFunction ? std::make_shared< PostponedTaskFunction >( std::move( taskFunction ) ) :
                              PostponedTaskFunctionPtr();
    }

    void onKeyChanged( const PostponedTaskKey& prevKey ) {
        m_key < prevKey ? heapifyDown( m_loop.m_tasks, m_heapIndex ) : heapifyUp( m_loop.m_tasks, m_heapIndex );
    }

    void startImpl( const Interval& first, const Interval& period, bool isPeriodic ) {
        const auto timePoint = m_loop.m_timeProvider.get().now() + first;
        std::unique_lock< std::mutex > locker( m_loop.m_mutex );

        m_period = period;
        m_isPeriodic = isPeriodic;

        if( m_isActive.exchange( true ) ) {
            removeFromWaitingsWithTimeout();
        }
        else if( auto cv = m_waitingCv.lock() ) {
            cv->notify_all();
        }

        const auto prevKey = m_key;
        m_key = std::make_pair( timePoint, ++m_loop.m_taskIndex );

        if( m_heapIndex == InvalidHeapIndex ) {
            addToHeap();
        }
        else {
            onKeyChanged( prevKey );
        }

        if( m_loop.m_unlimitedWaitingsCount || m_loop.m_tasks.front().m_task.get() == this ) {
            auto& cv = m_loop.m_unlimitedWaitingsCount ? m_loop.m_unlimitedWaitingCv : m_loop.m_timeoutWaitingCv;
            locker.unlock();
            cv.notify_one();
        }
    }

    void removeFromWaitingsWithTimeout() {
        auto found = m_loop.m_tasksOnWaiting.find( m_key );

        if( found != m_loop.m_tasksOnWaiting.end() ) {
            assert( found->second == shared_from_this() );
            found->second = nullptr;
        }
    }
    Interval getTimeToExecutionImpl() {
        const TaskLoop::TimePoint now = m_loop.m_timeProvider.get().now();
        std::lock_guard< std::mutex > locker( m_loop.m_mutex );
        const auto& timePoint = m_key.first;
        return timePoint.time_since_epoch() - now.time_since_epoch();
    }

private:
    TaskLoop& m_loop;
    PostponedTaskFunctionPtr m_taskFunctionPtr;  // using of shared ptr avoids copying on execution
    size_t m_callCount = 0;
    size_t m_heapIndex = InvalidHeapIndex;
    PostponedTaskKey m_key{ {}, 0 };
    std::atomic< Interval > m_period{ Interval{ 0 } };
    std::atomic< bool > m_isActive{ false };
    std::atomic< bool > m_isPeriodic{ false };
    std::atomic< PeriodicMode > m_periodicMode{ PeriodicMode::Default };
    std::weak_ptr< std::condition_variable > m_waitingCv;
};

TaskLoop::TaskItem::TaskItem( PostponedTaskImplPtr task )
: m_task( std::move( task ) )
, m_heapIndex( m_task->getHeapIndex() ) {}

bool TaskLoop::TaskItem::operator<( const TaskItem& other ) const {
    return m_task->getKey() < other.m_task->getKey();
}

TaskLoop::TaskItem& TaskLoop::TaskItem::operator=( TaskItem&& other ) noexcept {
    m_task = std::move( other.m_task );
    m_task->setHeapIndex( m_heapIndex );
    return *this;
}

TaskLoop::TaskLoop( size_t threadsCount )
: m_threadsCount( threadsCount )
, m_timeProvider( []() -> TimeProvider& {
    static TimeProviderImpl tp;
    return tp;
}() ) {}

TaskLoop::~TaskLoop() {
    if( m_terminationThread.joinable() ) {
        m_terminationThread.join();
    }

    std::unique_lock< std::mutex > locker( m_mutex );

    stopImpl( locker, StoppingMode::Auto );
}

PostponedTaskPtr TaskLoop::createPostponedTask( PostponedTaskFunction taskFunction ) {
    return std::make_shared< PostponedTaskImpl >( *this, std::move( taskFunction ) );
}

StrandId TaskLoop::postTask( std::function< void() >&& f ) {
    std::lock_guard< std::mutex > locker( m_strandsMutex );
    StrandId id = Strand::generateId();
    postTaskImpl( id, std::move( f ), std::move( locker ) );
    return id;
}

void TaskLoop::postTask( StrandId id, std::function< void() >&& f ) {
    std::lock_guard< std::mutex > locker( m_strandsMutex );
    postTaskImpl( id, std::move( f ), std::move( locker ) );
}

void TaskLoop::postTaskImpl( StrandId id, std::function< void() >&& f, std::lock_guard< std::mutex >&& /*locker*/ ) {
    auto it = m_strands.find( id );
    if( it == m_strands.end() ) {
        auto strand = Strand::createStrand( *this, id );
        it = m_strands.emplace( id, std::move( strand ) ).first;
    }
    it->second->post( std::move( f ) );
}

bool TaskLoop::isActiveThread( std::thread::id threadId ) {
    std::lock_guard< std::mutex > locker( m_mutex );
    return m_activeThreads.count( threadId ) != 0;
}

void TaskLoop::removeStrand( StrandId id ) {
    std::lock_guard< std::mutex > locker( m_strandsMutex );
    m_strands.erase( id );
}

void TaskLoop::stop( StoppingMode mode ) {
    if( mode == StoppingMode::OnTerminationSignalReceived ) {
        // We cannot lock m_mutex in signal handler to avoid dead lock.
        // Resetting of m_isRunning without mutex doesn't have strong stopping guarantees.
        // So we run thread that do the same with mutex and guarantees stopping.
        if( m_isRunning.exchange( false ) ) {
            notifyAll();

            if( !m_terminationThread.joinable() ) {
                m_terminationThread = std::thread( [this]() {
                    std::unique_lock< std::mutex > locker( m_mutex );

                    stopImpl( locker, StoppingMode::NoWaiting );
                } );
            }
        }
    }
    else {
        std::unique_lock< std::mutex > locker( m_mutex );
        stopImpl( locker, mode );
    }
}

void TaskLoop::stopImpl( std::unique_lock< std::mutex >& locker, StoppingMode mode ) {
    assert( mode != StoppingMode::OnTerminationSignalReceived );

    m_isRunning = false;
    locker.unlock();

    notifyAll();

    if( mode == StoppingMode::WaitingIfPossible ) {
        locker.lock();
        if( m_activeThreads.find( std::this_thread::get_id() ) == m_activeThreads.end() ) {
            m_waitingCv.wait( locker, [&]() { return m_activeThreads.empty() || m_isRunning; } );
        }
    }
}

void TaskLoop::run() {
    std::atomic< bool > exceptionCaught{ false };

    {
        std::vector< std::future< void > > futures;
        std::unique_lock< std::mutex > locker( m_mutex );

        if( !m_activeThreads.empty() || m_isRunning ) {
            return;
        }

        m_isRunning = true;

        futures.reserve( m_threadsCount );

        for( size_t i = 0; i < m_threadsCount; ++i ) {
            futures.push_back( std::async( std::launch::async, [&]() {
                std::unique_lock< std::mutex > locker( m_mutex );
                processTasks( locker, exceptionCaught );
            } ) );
        }

        processTasks( locker, exceptionCaught );
    }

    m_waitingCv.notify_all();

    if( exceptionCaught ) {
        throw TaskException{};
    }
}

void TaskLoop::processTasks( std::unique_lock< std::mutex >& locker, std::atomic< bool >& exceptionCaught ) {
    try {
        m_activeThreads.insert( std::this_thread::get_id() );

        auto removeThread = []( TaskLoop* loop ) { loop->m_activeThreads.erase( std::this_thread::get_id() ); };
        std::unique_ptr< TaskLoop, decltype( removeThread ) > activeThreadRemover( this, removeThread );

        while( m_isRunning ) {
            if( m_tasks.empty() ) {
                ++m_unlimitedWaitingsCount;
                m_unlimitedWaitingCv.wait( locker, [this]() { return !m_tasks.empty() || !m_isRunning; } );
                --m_unlimitedWaitingsCount;
                continue;
            }

            auto task = std::move( m_tasks.front().m_task );

            assert( task->getHeapIndex() == 0 );
            task->setHeapIndex( InvalidHeapIndex );

            if( m_tasks.size() > 1 ) {
                m_tasks.front() = std::move( m_tasks.back() );
                m_tasks.pop_back();
                heapifyUp( m_tasks, 0 );
            }
            else {
                m_tasks.pop_back();
            }

            const auto key = task->getKey();

            if( key.first > m_timeProvider.get().now() ) {
                m_tasksOnWaiting.emplace( key, std::move( task ) );

                const auto status = m_timeProvider.get().waitUntil( locker, m_timeoutWaitingCv, key.first );

                auto found = m_tasksOnWaiting.find( key );
                assert( found != m_tasksOnWaiting.end() );

                task = std::move( found->second );
                m_tasksOnWaiting.erase( found );

                if( !task ) {
                    continue;
                }

                assert( key == task->getKey() );

                if( status != std::cv_status::timeout || !m_isRunning ) {
                    task->addToHeap();
                    continue;
                }
            }

            assert( task->isActive() );

            PostponedTaskImpl::CallbackRunner( task, locker )();

            if( task ) {
                if( task->getKey().second != key.second ) {
                    continue;
                }

                if( task->isPeriodic() ) {
                    const auto interval = task->getInterval();
                    const auto shouldUseFixedPeriod =
                        interval.count() && task->getPeriodicMode() == PostponedTask::PeriodicMode::FixedPeriod;
                    task->setKey( { ( shouldUseFixedPeriod ? key.first : m_timeProvider.get().now() ) + interval,
                                    ++m_taskIndex } );
                    task->addToHeap();
                }
            }
        }
    }
    catch( const std::exception& error ) {
        Log::error( cc::error( "Exception '" ) + cc::note( typeid( error ).name() ) + cc::error( "' was caught:" ) +
                    cc::note( error.what() ) );

        exceptionCaught = true;
        stopImpl( locker, StoppingMode::NoWaiting );
    }
    catch( ... ) {
        exceptionCaught = true;
        stopImpl( locker, StoppingMode::NoWaiting );
    }
}

void TaskLoop::notifyAll() {
    m_timeoutWaitingCv.notify_all();
    m_unlimitedWaitingCv.notify_all();
}

bool TaskLoop::isRunning() {
    return m_isRunning;
}

}  // namespace Services
}  // namespace HubPlatform
