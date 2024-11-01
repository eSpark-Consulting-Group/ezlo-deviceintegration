#include <tiny/tiny_cxx_replacements.h>

#include <chrono>
#include <thread>
#include <cmath>
#include <sys/types.h>
#include <time.h>

#if( defined _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#endif  // (defined _WIN32)

namespace tiny {

void sleep_for_microseconds( uint64_t n ) {
#if( defined __BUILDING_4_MAC_OS_X__ )
    ::usleep( (useconds_t) n );
#else
    std::this_thread::sleep_for( std::chrono::microseconds( n ) );
#endif
}
void sleep_for_milliseconds( uint64_t n ) {
#if( defined __BUILDING_4_MAC_OS_X__ )
    sleep_for_microseconds( n / 1000 + ( ( ( n % 1000 ) != 0 ) ? 1 : 0 ) );
#else
    std::this_thread::sleep_for( std::chrono::milliseconds( n ) );
#endif
}
void sleep_for_seconds( uint64_t n ) {
#if( defined __BUILDING_4_MAC_OS_X__ )
    sleep_for_milliseconds( n / 1000 + ( ( ( n % 1000 ) != 0 ) ? 1 : 0 ) );
#else
    std::this_thread::sleep_for( std::chrono::seconds( n ) );
#endif
}

bool is_nan( float f ) {
    //        const uint32_t u = *(uint32_t*)&f;
    //        return (u&0x7F800000) == 0x7F800000 && (u&0x7FFFFF); // Both NaN and qNan
    return std::isnan( f );
}
bool is_nan( double d ) {
    //        const uint64_t u = *(uint64_t*)&d;
    //        return (u&0x7FF0000000000000ULL) == 0x7FF0000000000000ULL && (u&0xFFFFFFFFFFFFFULL);
    return std::isnan( d );
}

/*
    mutex::mutex() {
        ::pthread_mutex_init( &mutex_, nullptr );
    }
    mutex::mutex( const char * name ) {
        ::pthread_mutex_init( &mutex_, nullptr );
    }
    mutex::mutex( int type, const char * name ) {
        if( type == SHARED ) {
            pthread_mutexattr_t attr;
            ::pthread_mutexattr_init( &attr );
            ::pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
            ::pthread_mutex_init( &mutex_, &attr );
            ::pthread_mutexattr_destroy( &attr );
        } else
            ::pthread_mutex_init( &mutex_, nullptr );
    }
    mutex::~mutex() {
        ::pthread_mutex_destroy( &mutex_ );
    }
    status_t mutex::lock() {
        return - ::pthread_mutex_lock( &mutex_ );
    }
    void mutex::unlock() {
        pthread_mutex_unlock( &mutex_ );
    }
    status_t mutex::try_lock() {
        return - ::pthread_mutex_trylock( &mutex_ );
    }

    recursive_mutex::recursive_mutex() {
        pthread_mutexattr_t attr;
        ::pthread_mutexattr_init( &attr );
        ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
        ::pthread_mutex_init( &mutex_, &attr );
        ::pthread_mutexattr_destroy( &attr );
    }
    recursive_mutex::recursive_mutex( const char * name ) {
        pthread_mutexattr_t attr;
        ::pthread_mutexattr_init( &attr );
        ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
        ::pthread_mutex_init( &mutex_, &attr );
        ::pthread_mutexattr_destroy( &attr );
    }
    recursive_mutex::recursive_mutex( int type, const char * name ) {
        if( type == SHARED ) {
            pthread_mutexattr_t attr;
            ::pthread_mutexattr_init( &attr );
            ::pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
            ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
            ::pthread_mutex_init( &mutex_, &attr );
            ::pthread_mutexattr_destroy( &attr );
        } else {
            pthread_mutexattr_t attr;
            ::pthread_mutexattr_init( &attr );
            ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
            ::pthread_mutex_init( &mutex_, &attr );
            ::pthread_mutexattr_destroy( &attr );
        }
    }
    recursive_mutex::~recursive_mutex() {
        ::pthread_mutex_destroy( &mutex_ );
    }
    status_t recursive_mutex::lock() {
        return - ::pthread_mutex_lock( &mutex_ );
    }
    void recursive_mutex::unlock() {
        pthread_mutex_unlock( &mutex_ );
    }
    status_t recursive_mutex::try_lock() {
        return - ::pthread_mutex_trylock( &mutex_ );
    }

    condition_variable::condition_variable() {
        ::pthread_cond_init( &cond_, nullptr );
    }
    condition_variable::condition_variable( int type ) {
        if( type == SHARED ) {
            pthread_condattr_t attr;
            ::pthread_condattr_init( &attr );
            ::pthread_condattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
            ::pthread_cond_init( &cond_, &attr );
            ::pthread_condattr_destroy( &attr );
        } else
            ::pthread_cond_init( &cond_, nullptr );
    }
    condition_variable::~condition_variable() {
        ::pthread_cond_destroy( &cond_ );
    }
    status_t condition_variable::wait( mutex & m ) {
        return - ::pthread_cond_wait(&cond_, &m.mutex_ );
    }
    status_t condition_variable::waitRelative( mutex & m, nsecs_t reltime ) {
//#if (defined HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE)
//        struct timespec ts;
//        ts.tv_sec  = reltime/1000000000;
//        ts.tv_nsec = reltime%1000000000;
//        return - ::pthread_cond_timedwait_relative_np( &cond_, &m.mutex_, &ts );
//#else // HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
        struct timespec ts;
//#if defined(HAVE_POSIX_CLOCKS)
        ::clock_gettime( CLOCK_REALTIME, &ts );
//#else // HAVE_POSIX_CLOCKS
//        // we don't support the clocks here.
//        struct timeval t;
//        ::gettimeofday( &t, nullptr );
//        ts.tv_sec = t.tv_sec;
//        ts.tv_nsec= t.tv_usec*1000;
//#endif // HAVE_POSIX_CLOCKS
        ts.tv_sec += reltime/1000000000;
        ts.tv_nsec+= reltime%1000000000;
        if( ts.tv_nsec >= 1000000000 ) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec  += 1;
        }
        return - ::pthread_cond_timedwait( &cond_, &m.mutex_, &ts );
//#endif // HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
    }
    void condition_variable::signal() {
        ::pthread_cond_signal( &cond_ );
    }
    void condition_variable::broadcast() {
        ::pthread_cond_broadcast( &cond_ );
    }

    thread::thread()
        : t_( pthread_t(nullptr) )
        , arg_( nullptr )
    { }
    thread::thread( thread && t )
        : t_( pthread_t(nullptr) )
        , arg_( nullptr )
    {
        swap( t );
    }
    thread::~thread() {
        if( joinable() )
            std::terminate();
    }
    thread & thread::operator=( thread && t ) {
        if( joinable() )
            std::terminate();
        swap( t );
        return (*this);
    }
    void thread::swap( thread & t ) {
        pthread_t tt = t_;
        t_ = t.t_;
        t.t_ = tt;
        fn_t ff = fn_;
        fn_ = t.fn_;
        t.fn_ = ff;
        void * aa = arg_;
        arg_ = t.arg_;
        t.arg_ = aa;
    }
    bool thread::joinable() const {
        return ( t_ != pthread_t(nullptr) ) ? true : false;
    }
    void thread::join() {
        if( ! joinable() )
            return;
        ::pthread_join( t_, nullptr );
    }
    pthread_t thread::detach() {
        pthread_t x = t_;
        t_ = pthread_t(nullptr);
        fn_ = nullptr;
        arg_ = nullptr;
        return x;
    }
    void * thread::stat_thread_proc( void * opaque ) {
        thread * t = reinterpret_cast < thread * > ( opaque );
        return t->thread_proc();
    }
    void * thread::thread_proc() {
        return fn_( arg_ );
    }
    bool thread::create_fn( fn_t fn, void * arg = nullptr ) {
        if( joinable() )
            std::terminate();
        fn_ = fn;
        auto r = ::pthread_create( &t_, nullptr, stat_thread_proc, this );
        return ( r == 0 ) ? true : false;
    }

    mutex & get_interlocket_mutex() {
        static mutex x;
        return x;
    }
    thread get_this_thread() {
        thread t;
        t.t_ = pthread_self();
        return t;
    }
    namespace this_thread {
        void sleep_for_nanoseconds( const nsecs_t & sleep_duration ) {
#if (defined _WIN32)
            ::Sleep( sleep_duration * 1000 );
#else
            usleep( sleep_duration );
#endif // (defined _WIN32)

        }
    }; /// namespace this_thread
*/

};  // namespace tiny
