#pragma once

#include "service_interfaces/postponed_task.h"
#include "service_interfaces/strand_fwd.h"

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <map>
#include <vector>
#include <unordered_set>
#include <thread>

namespace HubPlatform {
namespace Services {

namespace UT {
class TaskLoopTestHelper;
}

class TaskLoop {
    using TimePoint = std::chrono::steady_clock::time_point;
    using TaskIndex = std::uint64_t;
    using PostponedTaskKey = std::pair< TimePoint, TaskIndex >;
    class PostponedTaskImpl;
    using PostponedTaskImplPtr = std::shared_ptr< PostponedTaskImpl >;
    using StrandMap = std::map< StrandId, StrandPtr >;

public:
    class TaskException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "TaskLoop: uncaught exceptions";
        }
    };

    using Interval = PostponedTask::Interval;
    using StoppingMode = PostponedTask::StoppingMode;

    TaskLoop( size_t threadsCount = 0 );

    virtual ~TaskLoop();

    virtual void stop( StoppingMode mode = StoppingMode::Auto );
    virtual void run();
    bool isRunning();

    template < typename Callback, decltype( std::declval< Callback >()() )* = nullptr >
    void addTask( Callback&& f, bool isPeriodic = false ) {
        createPostponedTask( std::bind( std::forward< Callback >( f ) ) )->start( Interval( 0 ), isPeriodic );
    }
    PostponedTaskPtr createPostponedTask( PostponedTaskFunction taskFunction );

    StrandId postTask( std::function< void() >&& f );
    void postTask( StrandId id, std::function< void() >&& f );

private:
    void stopImpl( std::unique_lock< std::mutex >& locker, StoppingMode mode );

    void processTasks( std::unique_lock< std::mutex >& locker, std::atomic< bool >& exceptionCaught );

    void notifyAll();

    bool isActiveThread( std::thread::id threadId );

    void removeStrand( StrandId id );

    void postTaskImpl( StrandId id, std::function< void() >&& f, std::lock_guard< std::mutex >&& locker );

    class TimeProvider {
    public:
        virtual ~TimeProvider() = default;
        virtual TimePoint now() const = 0;
        virtual std::cv_status waitUntil( std::unique_lock< std::mutex >& locker,
                                          std::condition_variable& condVar,
                                          const TimePoint& timePoint ) = 0;
    };

    class TimeProviderImpl;

    struct TaskItem {
        PostponedTaskImplPtr m_task;
        const size_t m_heapIndex;

        bool operator<( const TaskItem& ) const;

        TaskItem( PostponedTaskImplPtr task );

        TaskItem( const TaskItem& ) = delete;
        TaskItem( TaskItem&& ) noexcept = default;
        TaskItem& operator=( const TaskItem& ) = delete;
        TaskItem& operator=( TaskItem&& ) noexcept;
    };

private:
    size_t m_threadsCount;
    std::reference_wrapper< TimeProvider > m_timeProvider;
    std::vector< TaskItem > m_tasks;
    std::map< PostponedTaskKey, PostponedTaskImplPtr > m_tasksOnWaiting;
    TaskIndex m_taskIndex = 0;
    std::condition_variable m_timeoutWaitingCv;
    std::condition_variable m_unlimitedWaitingCv;
    size_t m_unlimitedWaitingsCount = 0;
    std::condition_variable m_waitingCv;
    std::mutex m_mutex;
    std::unordered_set< std::thread::id > m_activeThreads;
    std::atomic< bool > m_isRunning{ false };

    std::mutex m_strandsMutex;
    StrandMap m_strands;

    std::thread m_terminationThread;

    friend class Strand;
    friend class UT::TaskLoopTestHelper;
};

}  // namespace Services
}  // namespace HubPlatform
