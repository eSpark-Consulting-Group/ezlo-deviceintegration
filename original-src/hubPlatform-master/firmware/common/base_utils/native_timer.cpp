#include "base_utils/native_timer.h"

#include <signal.h>
#include <cassert>

class NativeTimer::Runner {
public:
    Runner( NativeTimer& owner )
    : m_owner( owner )
    , m_isStopped( m_owner.isStopped() ) {
        std::unique_lock< std::mutex > locker( m_owner.m_mutex );

        m_isStopped = m_isStopped || m_owner.isStopped();  // double check

        if( !m_isStopped ) {
            ++m_owner.m_thrCount;
        }
    }

    void operator()() const {
        if( !m_isStopped ) {
            ++m_owner.m_counter;
            if( const auto& cb = m_owner.m_callback ) {
                cb( m_owner );
            }
        }
    }

    ~Runner() {
        if( !m_isStopped ) {
            std::unique_lock< std::mutex > locker( m_owner.m_mutex );
            assert( m_owner.m_thrCount );
            if( --m_owner.m_thrCount == 0 ) {
                locker.unlock();
                m_owner.m_thrCountCondVar.notify_all();
            }
        }
    }

private:
    NativeTimer& m_owner;
    bool m_isStopped;
};

NativeTimer::NativeTimer( Callback cb ) noexcept
: m_callback( std::move( cb ) ) {}

NativeTimer::~NativeTimer() {
    stop( StoppingMode::WaitingForThreads );
}

bool NativeTimer::start( Interval interval, bool isSingleShot ) noexcept {
    return start( interval, isSingleShot ? Interval() : interval );
}

size_t NativeTimer::getCounter() const noexcept {
    return m_counter;
}

bool NativeTimer::start( Interval firstInterval, Interval periodicInterval ) noexcept {
    std::unique_lock< std::mutex > locker( m_mutex );

    if( m_waitings != 0 ) {
        return false;
    }

    bool result = false;

    m_counter = 0;
    if( auto timer = enshureTimerCreated() ) {
        auto makeTimespec = []( const Interval& interval ) {
            const auto sec = std::chrono::duration_cast< std::chrono::seconds >( interval );
            const auto nsec = std::chrono::duration_cast< std::chrono::nanoseconds >( interval - sec );

            return timespec{ static_cast< decltype( timespec::tv_sec ) >( sec.count() ),
                             static_cast< decltype( timespec::tv_nsec ) >( nsec.count() ) };
        };

        const itimerspec timer_specs = { makeTimespec( periodicInterval ),
                                         makeTimespec( firstInterval.count() ? firstInterval : periodicInterval ) };

        result = timer_settime( timer, 0, &timer_specs, nullptr ) == 0;
    }

    m_isStopped = false;

    return result;
}

void NativeTimer::stop( StoppingMode stoppingMode ) noexcept {
    std::unique_lock< std::mutex > locker( m_mutex );

    m_isStopped = true;

    if( !m_timer ) {
        return;
    }

    if( m_waitings == 0 ) {
        const itimerspec timer_specs = {};
        timer_settime( m_timer, 0, &timer_specs, nullptr );
    }

    if( stoppingMode == StoppingMode::Default ) {
        return;
    }

    ++m_waitings;

    while( m_thrCount ) {
        assert( m_isStopped );
        m_thrCountCondVar.wait( locker );  // 1 sec - waiting threshold (we can't use mutex in timer thread)
    }

    --m_waitings;

    // check again
    if( !m_timer ) {
        return;
    }

    // Reason of timer_delete:
    // we have to provide stong guarantee of threads finishing in case of timer start after stopping
    // timer.stop(StoppingMode::WaitingForThreads);
    // timer.start(...);

    timer_delete( m_timer );
    m_timer = nullptr;
    m_thrCount = 0;

    locker.unlock();
    m_thrCountCondVar.notify_all();
}

bool NativeTimer::isRunning() const noexcept {
    Interval leavedTime;
    getIntervals( &leavedTime );
    return leavedTime.count() != 0;
}

bool NativeTimer::isStopped() const noexcept {
    return m_isStopped;
}

bool NativeTimer::isPeriodic() const noexcept {
    Interval periodicTime;
    getIntervals( nullptr, &periodicTime );
    return periodicTime.count() != 0;
}

void NativeTimer::getIntervals( Interval* leaved, Interval* periodic ) const noexcept {
    itimerspec timespec = {};

    {
        std::unique_lock< std::mutex > locker( m_mutex );

        if( timer_t timer = m_timer ) {
            timer_gettime( timer, &timespec );
        }
    }

    auto getInterval = []( Interval* interval, const struct timespec& t ) {
        if( interval ) {
            *interval = std::chrono::nanoseconds( t.tv_nsec ) + std::chrono::seconds( t.tv_sec );
        }
    };

    getInterval( leaved, timespec.it_value );
    getInterval( periodic, timespec.it_interval );
}

timer_t NativeTimer::enshureTimerCreated() {
    if( !m_timer ) {
        sigevent timer_event = {};
        timer_event.sigev_notify = SIGEV_THREAD;
        timer_event.sigev_value.sival_ptr = this;
        timer_event.sigev_notify_function = []( sigval val ) {
            Runner runner( *reinterpret_cast< NativeTimer* >( val.sival_ptr ) );
            runner();
        };

        const auto creationResult = timer_create( CLOCK_MONOTONIC, &timer_event, &m_timer );
        assert( creationResult == 0 );
    }

    return m_timer;
}
