#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <time.h>

/* @brief Safe wrapper of linux native timer
 */
class NativeTimer {
public:
    using Interval = std::chrono::nanoseconds;
    using Callback = std::function< void( NativeTimer& ) >;

    enum class StoppingMode {
        Default,
        WaitingForThreads,
    };

    template < typename Cb, decltype( std::declval< Cb >()() )* = nullptr >
    NativeTimer( Cb&& cb );

    /* @brief Constructor
     * @params[in] callback - callback of timer notification.
     * @note Callback will be called in separate thread.
     */
    NativeTimer( Callback callback ) noexcept;

    NativeTimer( const NativeTimer& ) = delete;
    NativeTimer& operator=( const NativeTimer& ) = delete;

    ~NativeTimer();

    /* @brief starts timer with custom first and periodic interval. Stops timer if zero intervals passed
     * @param[in] firstInterval - interval between current time and first timer notification. Zero value means that
     * periodicInterval wiil be used instead.
     * @param[in] periodicInterval - interval between timer notifications. Zero value means non-periodic call.
     */
    bool start( Interval firstInterval, Interval periodicInterval ) noexcept;
    bool start( Interval interval, bool isSingleShot = false ) noexcept;

    /* @brief stops timer.
     * @param[in] stoppingMode - descries wait threads or not.
     * @note Don't use StoppingMode::WaitingForThreads from callback (otherwise yo will have deadlock)
     */
    void stop( StoppingMode stoppingMode = StoppingMode::Default ) noexcept;

    /*@brief Checks is timer running. Timer is not running after stop or after single shot notification.
     */
    bool isRunning() const noexcept;

    /*@brief Checks is timer periodic.
     */
    bool isPeriodic() const noexcept;

    /*@brief Checks is stop was called. Method 'start' reset stopped status.
     */
    bool isStopped() const noexcept;

    /*@brief get leaved time to callback execution and periodic interval (zero if not periodic)
     */
    void getIntervals( Interval* leaved, Interval* periodic = nullptr ) const noexcept;

    template < typename Duration >
    void getIntervals( Duration* leaved, Duration* periodic = nullptr ) const noexcept;

    /*@brief Gets how many ticks was occured after timer start.
     */
    size_t getCounter() const noexcept;

private:
    timer_t enshureTimerCreated();

private:
    class Runner;

    timer_t m_timer = nullptr;
    size_t m_thrCount = 0u;
    std::atomic< size_t > m_counter{ 0u };
    size_t m_waitings = 0u;
    std::atomic< bool > m_isStopped{ true };

    Callback m_callback;
    std::condition_variable m_thrCountCondVar;
    mutable std::mutex m_mutex;
};

// inline methods

template < typename Cb, decltype( std::declval< Cb >()() )* >
NativeTimer::NativeTimer( Cb&& cb )
: NativeTimer( Callback( std::bind( std::forward< Cb >( cb ) ) ) ) {}

template < typename Duration >
void NativeTimer::getIntervals( Duration* leaved, Duration* periodic ) const noexcept {
    Interval nsecLeaved, nsecPeriodic;

    getIntervals( leaved ? &nsecLeaved : nullptr, periodic ? &nsecPeriodic : nullptr );
    if( leaved ) {
        *leaved = std::chrono::duration_cast< Duration >( nsecLeaved );
    }
    if( periodic ) {
        *periodic = std::chrono::duration_cast< Duration >( nsecPeriodic );
    }
}
