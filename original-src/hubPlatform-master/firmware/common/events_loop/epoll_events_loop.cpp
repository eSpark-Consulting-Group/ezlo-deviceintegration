#include "events_loop/epoll_events_loop.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

#include <array>
#include <sstream>
#include <cassert>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

namespace HubPlatform {
namespace EventsLoop {

class EventsLooptError : public std::runtime_error {
public:
    EventsLooptError( const std::string& errDesription )
    : runtime_error( "EventsLoop: " + errDesription ) {}
};

IEventsLoop::~IEventsLoop() = default;

EventType operator|( EventType a, EventType b ) {
    return static_cast< EventType >( static_cast< int >( a ) | static_cast< int >( b ) );
}

EventType operator|=( EventType a, EventType b ) {
    return static_cast< EventType >( static_cast< int >( a ) | static_cast< int >( b ) );
}

EventType operator&( EventType a, EventType b ) {
    return static_cast< EventType >( static_cast< int >( a ) & static_cast< int >( b ) );
}

EventType operator&=( EventType a, EventType b ) {
    return static_cast< EventType >( static_cast< int >( a ) & static_cast< int >( b ) );
}

namespace {

/* exception used for safely thread termination (calling pthread_exit() causes an immediate
 * termination of a thread thus preventing c++ destructors for automatic variables from being called,
 * so we throw an exception when we need to terminate a thread, thus allowing c++ runtime
 * to destroy all automatic variables and only after that call pthread_exit()) */
class EpollStopExecutionException final {};

std::string getEpollEventsString( int epollEvents ) {
    std::string result;

    if( epollEvents == 0 )
        return result;

    if( epollEvents & EPOLLIN )
        result += "EPOLLIN | ";
    if( epollEvents & EPOLLPRI )
        result += "EPOLLPRI | ";
    if( epollEvents & EPOLLOUT )
        result += "EPOLLOUT | ";
    if( epollEvents & EPOLLRDNORM )
        result += "EPOLLRDNORM | ";
    if( epollEvents & EPOLLRDBAND )
        result += "EPOLLRDBAND | ";
    if( epollEvents & EPOLLWRNORM )
        result += "EPOLLWRNORM | ";
    if( epollEvents & EPOLLWRBAND )
        result += "EPOLLWRBAND | ";
    if( epollEvents & EPOLLMSG )
        result += "EPOLLMSG | ";
    if( epollEvents & EPOLLERR )
        result += "EPOLLERR | ";
    if( epollEvents & EPOLLHUP )
        result += "EPOLLHUP | ";
    if( epollEvents & EPOLLRDHUP )
        result += "EPOLLRDHUP | ";
    if( epollEvents & EPOLLONESHOT )
        result += "EPOLLONESHOT | ";
    if( epollEvents & EPOLLET )
        result += "EPOLLET | ";

    result.erase( result.size() - 3 ); /* remove a last extra " | " substring */

    return result;
}

uint32_t castEventType( EventType eventType ) {
    switch( eventType ) {
    case EventType::IN:
        return EPOLLIN;
    case EventType::OUT:
        return EPOLLOUT;
    case EventType::RDHUP:
        return EPOLLRDHUP;
    default:
        assert( false );
        break;
    }

    return 0; /* to bypass a gcc warning */
}

EventType castEpollType( uint32_t eventType ) {
    EventType result;

    if( eventType == 0 )
        assert( false );

    if( eventType & EPOLLIN )
        result |= EventType::IN;
    if( eventType & EPOLLOUT )
        result |= EventType::OUT;
    if( eventType & EPOLLRDHUP )
        result |= EventType::RDHUP;
    if( eventType & EPOLLERR )
        result |= EventType::ERR;
    if( eventType & EPOLLHUP )
        result |= EventType::HUP;

    return EventType::NONE; /* to bypass a gcc warning */
}

}  // namespace

EpollEventsLoop::EventSource::EventSource()
: eventSourceHandle( -1 )
, eventTypes( EventType::NONE )
, fileDescriptor( -1 )
, armed( false )
, paused( false ) {}

EpollEventsLoop::EpollEventsLoop()
: m_eventSourceCounter( 0 ) {
    m_epollFileDescriptor = epoll_create( 1 );
    if( m_epollFileDescriptor < 0 )
        throw EventsLooptError{ "fail to create an epoll object" };

    m_eventsLoopThread = std::thread( &EpollEventsLoop::handleEvents, this );
    m_isThreadLaunched = true;
}

EpollEventsLoop::~EpollEventsLoop() {
    int eventFileDescriptor;

    if( !m_isThreadLaunched )
        return;

    /* trigger an interrupt event */
    eventFileDescriptor = eventfd( 1, 0 );
    addEventSource( eventFileDescriptor, EventType::IN, []( EventType eventType, EventSourceHandle evSrcHndl ) {
        throw EpollStopExecutionException();
    } );

    /* wait till the thread exits */
    if( m_eventsLoopThread.joinable() )
        m_eventsLoopThread.join();

    close( eventFileDescriptor );
    close( m_epollFileDescriptor );
}

EpollEventsLoop::EpollEventsLoop( EpollEventsLoop&& rhs ) {
    std::lock_guard< std::mutex > lock( rhs.m_eventSourcesMtx );

    m_epollFileDescriptor = rhs.m_epollFileDescriptor;
    m_eventsLoopThread = std::move( rhs.m_eventsLoopThread );
    m_eventSources = std::move( rhs.m_eventSources );
    m_eventSourceCounter = rhs.m_eventSourceCounter;

    /* TODO: why this assignment does trigger an epoll_wait()??? */
    rhs.m_epollFileDescriptor = -1;

    m_isThreadLaunched = true;
    rhs.m_isThreadLaunched = false;
}

void EpollEventsLoop::handleEvents() {
    /* the thread gets interrupted by a special event source, look at a dtor */
    while( true ) {
        /* TODO: it's a big question what to do with such exceptions... */
        try {
            std::array< epoll_event, 64 > events;
            int evsAmount;

            /* blocks till any event happens */
            evsAmount = epoll_wait( m_epollFileDescriptor, &events.front(), events.size(), -1 );
            if( evsAmount < 0 &&
                errno == EINTR ) /* TODO: maybe it's more preferable just to block an INTERRUPT signal? */
                continue;

            if( evsAmount < 0 ) {
                Log::error( cc::error( "EventsLoop: fail to make an epoll_wait() syscall" ) );
                assert( false ); /* TODO: handle this case */
            }

            std::unique_lock< std::mutex > lock( m_eventSourcesMtx );

            /* call registered event handlers */
            for( auto i = 0; i < evsAmount; i++ ) {
                const auto* eventSource = static_cast< EventSource* >( events[ i ].data.ptr );

                if( Log::getLogLevel() >= Log::TRACE ) {
                    std::stringstream stringStream;
                    stringStream << cc::trace( "EventsLoop: remove an event source : " ) << cc::note() << eventSource
                                 << cc::reset() << cc::trace( ", fd: " ) << cc::num10( eventSource->fileDescriptor )
                                 << cc::trace( ", ev_type: " ) << cc::note( getEpollEventsString( events[ i ].events ) )
                                 << cc::trace( ", armed: " ) << cc::note() << eventSource->armed << cc::reset();
                    LOGGER_TRACE( cc::trace( stringStream.str() ) );
                }

                /* epoll_wait may return several events by one call, during handling them
                 * a removeEventSource() function can be called which means that a client
                 * doens't want to be notified about that event so just apply that demand and
                 * skip handling an event marked as disarmed */
                if( !eventSource->armed )
                    continue;

                /* epoll_wait may return several events by one call, during handling them
                 * apauseEventSource() function can be called which means that a client
                 * doens't want to be notified about that event so just apply that demand and
                 * skip handling an event marked as paused */
                if( eventSource->paused )
                    continue;

                auto eventSourceHandler = eventSource->eventSourceHandle;
                auto eventHandler = eventSource->eventHandler;

                lock.unlock();
                eventHandler( castEpollType( events[ i ].events ), eventSourceHandler );
                lock.lock();
            }

            /* free the memory for disarmed event sources, as they won't be accessed anymore */
            for( auto it = m_eventSources.cbegin(); it != m_eventSources.cend(); ) {
                const auto& eventSource = it->second;

                if( !eventSource.armed ) {
                    if( Log::getLogLevel() >= Log::TRACE ) {
                        std::stringstream stringStream;
                        stringStream << cc::trace( "EventsLoop: remove an event source : " ) << cc::note()
                                     << &eventSource << cc::reset() << cc::trace( ", fd: " )
                                     << cc::num10( eventSource.fileDescriptor ) << cc::trace( ", armed: " )
                                     << cc::note() << eventSource.armed << cc::reset();
                        Log::trace( stringStream.str() );
                    }

                    it = m_eventSources.erase( it );
                }
                else
                    it++;
            }
        }
        catch( const EpollStopExecutionException& /*exception*/ ) {
            Log::debug( cc::debug( "EventsLoop: terminate the polling thread" ) );
            // just in case, pthrea_exit() is implicitly called at the end of a thread function anyway
            pthread_exit( nullptr );
        }
        catch( const std::exception& exception ) {
            Log::error( cc::error( "EventsLoop: " ) + cc::note( exception.what() ) );
        }
    }
}

EpollEventsLoop::EventSourceHandle EpollEventsLoop::addEventSource( int fileDescriptor,
                                                                    EventType eventType,
                                                                    EventHandler&& eventHandler ) {
    std::lock_guard< std::mutex > lock( m_eventSourcesMtx );

    epoll_event event;

    auto result = m_eventSources.emplace( std::make_pair( m_eventSourceCounter++, EventSource{} ) );
    if( !result.second )
        throw EventsLooptError{ std::string( "fail to add an event source" ) };

    auto& eventSource = result.first->second;
    eventSource.eventSourceHandle = result.first->first;
    eventSource.eventHandler = std::move( eventHandler );
    eventSource.eventTypes = eventType;
    eventSource.fileDescriptor = fileDescriptor;
    eventSource.armed = true;

    /* epoll_ctl copies an epoll_event structure to the kernel, so we have no choice...;
     * stl associative containers guarantee pointers validation till a pointed element gets removed */
    event.data.ptr = &eventSource;
    event.events = castEventType( eventType );

    if( epoll_ctl( m_epollFileDescriptor, EPOLL_CTL_ADD, eventSource.fileDescriptor, &event ) < 0 ) {
        m_eventSources.erase( result.first->first );
        throw EventsLooptError{ std::string( "fail to add an event source to epoll: " ) + strerror( errno ) };
    }

    return eventSource.eventSourceHandle;
}

void EpollEventsLoop::removeEventSource( EventSourceHandle eventSourceHandle ) {
    std::lock_guard< std::mutex > lock( m_eventSourcesMtx );

    auto it = m_eventSources.find( eventSourceHandle );
    if( it == m_eventSources.end() )
        return;

    auto& eventSource = it->second;

    if( eventSource.paused || !eventSource.armed ) {
        return;
    }

    /* TODO: we hope that the next epoll_wait won't return this event even if it happened
     * before/after a fd (event) was disarmed :), should be checked... */
    if( epoll_ctl( m_epollFileDescriptor, EPOLL_CTL_DEL, eventSource.fileDescriptor, nullptr ) < 0 )
        throw EventsLooptError{ std::string( "fail to remove an event source: " ) + strerror( errno ) };

    /* mark the event source as disarmed to free a memory within handleEvents() */
    eventSource.armed = false;

    if( Log::getLogLevel() >= Log::TRACE ) {
        std::stringstream stringStream;
        stringStream << cc::trace( "EventsLoop: remove an event source : " ) << cc::note() << &eventSource
                     << cc::reset() << cc::trace( ", fd: " ) << cc::num10( eventSource.fileDescriptor )
                     << cc::trace( ", armed: " ) << cc::note() << eventSource.armed << cc::reset();
        Log::trace( stringStream.str() );
    }

    /* an EventSource object can't be freed immediately here 'cause kernel keeps pointer to it
     * and may return it via epoll_wait() (this is true during events handling) */
}

void EpollEventsLoop::pauseEventSource( EventSourceHandle eventSourceHandle ) {
    std::lock_guard< std::mutex > lock( m_eventSourcesMtx );

    auto it = m_eventSources.find( eventSourceHandle );
    if( it == m_eventSources.end() )
        return;

    auto& eventSource = it->second;

    if( eventSource.paused )
        return;

    if( epoll_ctl( m_epollFileDescriptor, EPOLL_CTL_DEL, eventSource.fileDescriptor, nullptr ) < 0 )
        throw EventsLooptError{ std::string( "fail to pause an event source: " ) + strerror( errno ) };

    /* epoll_wait might fetch this event already */
    eventSource.paused = true;

    std::stringstream stringStream;
    stringStream << "EventsLoop: pause an event source - event source: " << &eventSource
                 << ", fd: " << eventSource.fileDescriptor;
    Log::debug( stringStream.str() );
}

void EpollEventsLoop::resumeEventSource( EventSourceHandle eventSourceHandle ) {
    std::lock_guard< std::mutex > lock( m_eventSourcesMtx );

    epoll_event event;

    auto it = m_eventSources.find( eventSourceHandle );
    if( it == m_eventSources.end() )
        return;

    auto& eventSource = it->second;

    if( !eventSource.paused )
        return;

    event.data.ptr = &eventSource;
    event.events = castEventType( eventSource.eventTypes );

    if( epoll_ctl( m_epollFileDescriptor, EPOLL_CTL_ADD, eventSource.fileDescriptor, &event ) < 0 )
        throw EventsLooptError{ std::string( "fail to resume an event source: " ) + strerror( errno ) };

    eventSource.paused = false;

    std::stringstream stringStream;
    stringStream << "EventsLoop: resume an event source - event source: " << &eventSource
                 << ", fd: " << eventSource.fileDescriptor;
    Log::debug( stringStream.str() );
}

std::unique_ptr< IEventsLoop > makeEpollEventsLoop() {
    return std::make_unique< EpollEventsLoop >();
}

}  // namespace EventsLoop
}  // namespace HubPlatform
