#pragma once
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Events {

template < typename EventHandlerT, typename EventDataT >
class EventEmitter {
public:
    using EventHandler = EventHandlerT;
    using EventData = EventDataT;

public:
    virtual ~EventEmitter() = default;

    virtual void on( const std::string& eventName, EventHandler&& eventHandler ) {
        Locker lock( m_mutex );

        auto found = m_events.find( eventName );
        if( found == m_events.end() ) {
            CallList calls;
            calls.emplace_back( std::move( eventHandler ) );
            m_events.emplace( eventName, calls );
        }
        else {
            found->second.emplace_back( eventHandler );
        }
    }

    /** Removes all listeners for event "eventName" */
    virtual void off( const std::string& eventName, EventHandler&& eventHandler ) {
        Locker lock( m_mutex );

        auto found = m_events.find( eventName );
        if( found != m_events.end() ) {
            // remove all listeners
            found->second.clear();
        }
    }

    virtual void emit( const std::string& eventName, const std::string& eventHandler, const EventData& eventData ) {
        Locker lock( m_mutex );

        auto found = m_events.find( eventName );
        if( found == m_events.end() )
            return;

        const CallList& calls = found->second;

        for( auto call : calls ) {
            try {
                // call every handler in a call list.
                call( eventHandler, eventData );
            }
            catch( std::exception& err ) {
                if( m_verbose ) {
                    Log::error( cc::error( "EventEmitter handler error: '" ) + eventName + std::string( "': " ) +
                                err.what() );
                }
            }
        }
    }

protected:
    using CallList = std::vector< EventHandler >;
    using EventMap = std::map< std::string, CallList >;

    using Mutex = std::recursive_mutex;
    using Locker = std::lock_guard< Mutex >;
    mutable Mutex m_mutex;
    EventMap m_events;
    bool m_verbose = false;
};

}  // namespace Events
}  // namespace HubPlatform