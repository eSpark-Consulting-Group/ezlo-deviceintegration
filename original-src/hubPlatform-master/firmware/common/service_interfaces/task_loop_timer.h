#pragma once
#include "service_interfaces/postponed_task.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Services {

/* @brief Light-weight wrapper around PostponedTask with auto stopping in destructor
 */
class TaskLoopTimer {
public:
    using Interval = PostponedTask::Interval;
    using StoppingMode = PostponedTask::StoppingMode;
    using PeriodicMode = PostponedTask::PeriodicMode;

    template < typename TaskLoop >
    TaskLoopTimer( TaskLoop& loop )
    : m_postponedTask( loop.createPostponedTask( {} ) ) {}

    template < typename TaskLoop, typename Function /* void() or void(TaskLoopTimer&) */ >
    TaskLoopTimer( TaskLoop& loop, Function&& function )
    : m_postponedTask( loop.createPostponedTask( getTaskFunction( std::forward< Function >( function ) ) ) ) {}

    TaskLoopTimer( TaskLoopTimer&& ) = delete;
    TaskLoopTimer& operator=( const TaskLoopTimer& ) = delete;
    TaskLoopTimer& operator=( TaskLoopTimer&& ) = delete;
    TaskLoopTimer( const TaskLoopTimer& ) = delete;

    ~TaskLoopTimer() {
        stop();
    }

    void start( const Interval& first, const Interval& periodic ) {
        m_postponedTask->start( first, periodic );
    }

    void start( const Interval& interval, bool isPeriodic = true ) {
        m_postponedTask->start( interval, isPeriodic );
    }

    void stop( StoppingMode mode = StoppingMode::WaitingIfPossible ) {
        m_postponedTask->stop( mode );
    }

    bool isActive() const {
        return m_postponedTask->isActive();
    }
    bool isPeriodic() const {
        return m_postponedTask->isPeriodic();
    }

    Interval getInterval() const {
        return m_postponedTask->getInterval();
    }

    void setPeriodicMode( PeriodicMode mode ) {
        m_postponedTask->setPeriodicMode( mode );
    }

    template < typename Function /* void() or void(TaskLoopTimer&) */ >
    void setCallback( Function&& function ) {
        m_postponedTask->setCallback( getTaskFunction( std::forward< Function >( function ) ) );
    }

private:
    template < typename F, typename... Args >
    static auto call( F&& f, Args&&... args ) -> decltype( f( args... ) )*;
    static void call( ... );

    template < typename F >
    auto getTaskFunction( F&& f )
        -> std::enable_if_t< !std::is_void< decltype( call( f ) ) >::value, PostponedTaskFunction > {
        return std::bind( std::forward< F >( f ) );
    }

    template < typename F >
    auto getTaskFunction( F&& f ) -> std::enable_if_t< !std::is_void< decltype( call( f, *this ) ) >::value &&
                                                           std::is_void< decltype( call( f ) ) >::value,
                                                       PostponedTaskFunction > {
        return std::bind( std::forward< F >( f ), std::ref( *this ) );
    }

private:
    PostponedTaskPtr m_postponedTask;
};

}  // namespace Services
}  // namespace HubPlatform
