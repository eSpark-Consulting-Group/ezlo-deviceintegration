#pragma once

#include "service_interfaces/task_loop.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "service_interfaces/strand_fwd.h"

#include <queue>
#include <functional>
#include <cassert>

namespace HubPlatform {
namespace Services {

///
/// \brief The Strand class
/// A strand serializes handler execution.
/// It guarantees that no handlers execute concurrently.
/// Based on http://www.crazygaze.com/blog/2016/03/17/how-strands-work-and-why-you-should-use-them/
///
class Strand : public std::enable_shared_from_this< Strand > {
protected:
    Strand( TaskLoop& taskLoop, StrandId id )
    : m_taskLoop( taskLoop )
    , m_id( id ) {}

public:
    static StrandId generateId();
    static std::shared_ptr< Strand > createStrand( TaskLoop& taskLoop, StrandId id );

    Strand( const Strand& ) = delete;
    Strand& operator=( const Strand& ) = delete;

    // Executes the handler immediately if all the strand guarantees are met,
    // or posts the handler for later execution if the guarantees are not met
    // from inside this call
    template < typename F >
    void dispatch( F handler ) {
        // If we are not currently in the processor dispatching function (in
        // this thread), then we cannot possibly execute the handler here, so
        // enqueue it and bail out
        if( !m_taskLoop.isActiveThread( std::this_thread::get_id() ) ) {
            post( std::move( handler ) );
            return;
        }

        // NOTE: At this point we know we are in a worker thread (because of the
        // check above)

        // If we are running the strand in this thread, then we can execute the
        // handler immediately without any other checks, since by design no
        // other threads can be running the strand
        if( runningInThisThread() ) {
            handler();
            return;
        }

        // At this point we know we are in a worker thread, but not running the
        // strand in this thread.
        // The strand can still be running in another worker thread, so we need
        // to atomically enqueue the handler for the other thread to execute OR
        // mark the strand as running in this thread
        auto trigger = m_data.process( [&]( Data& data ) {
            if( data.running ) {
                data.q.push( std::move( handler ) );
                return false;
            }
            else {
                data.running = true;
                return true;
            }
        } );

        if( trigger ) {
            // Add a marker to the callstack, so the handler knows the strand is
            // running in the current thread
            m_threadId = std::this_thread::get_id();
            handler();

            // Run any remaining handlers.
            // At this point we own the strand (It's marked as running in
            // this thread), and we don't release it until the queue is empty.
            // This means any other threads adding handlers to the strand will
            // enqueue them, and they will be executed here.
            run();
        }
    }

    // Post an handler for execution and returns immediately.
    // The handler is never executed as part of this call.
    template < typename F >
    void post( F handler ) {
        // We atomically enqueue the handler AND check if we need to start the
        // running process.
        bool trigger = m_data.process( [&handler]( Data& data ) {
            data.q.push( std::move( handler ) );
            if( data.running ) {
                return false;
            }
            else {
                data.running = true;
                return true;
            }
        } );

        // The strand was not running, so trigger a run
        if( trigger ) {
            // We cannot simply destroy strand object because posted tasks
            // may be still executing, so shared ptr prevents destroying until
            // run() is finished
            auto self = shared_from_this();
            m_taskLoop.addTask( [self] { self->run(); } );
        }
    }

    bool runningInThisThread() {
        return m_threadId == std::this_thread::get_id();
    }

private:
    // Processes any enqueued handlers.
    // This assumes the strand is marked as running.
    // When there are no more handlers, it marks the strand as not running.
    void run();

private:
    std::atomic< std::thread::id > m_threadId;

    struct Data {
        bool running = false;
        std::queue< std::function< void() > > q;
    };
    threads::UniqueSafeWrapper< Data > m_data;

    TaskLoop& m_taskLoop;
    StrandId m_id;
};

}  // namespace Services
}  // namespace HubPlatform
