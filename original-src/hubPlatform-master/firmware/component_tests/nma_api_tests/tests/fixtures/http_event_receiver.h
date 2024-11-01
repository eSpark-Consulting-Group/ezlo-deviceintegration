#pragma once
#include "json_utils/json_fwd.h"
#include "tiny/tiny_utils.h"
#include "http/http.h"
#include "http/resources.h"
#include <deque>
#include <string>
#include <mutex>
#include <atomic>
#include <utility>

namespace HubPlatform {
namespace Tests {

struct EventReceiver {
    EventReceiver( CommonHttp::Http& http, std::string pluginName )
    : m_http( http )
    , m_pluginName( std::move( pluginName ) ) {}

    void handleDataReceivedAndClose( const std::string /*handler*/&, CommonHttp::EventPtr eventPtr ) {
        // if data received, close connection immediately
        if( !eventPtr )
            throw std::logic_error( "Null-pointer exception" );
        std::unique_lock< std::mutex > lock( m_mutex );
        const char* eventName = CommonHttp::Resources::HttpEvents::HttpDataReceived;
        m_events.push_back( { eventName, eventPtr->toJson() } );
        lock.unlock();

        m_condVar.notify_all();
        LOGGER_INFO( cc::note( eventName ) + cc::trace( " event received; close connection " ) + eventPtr->id() );
        m_http.closeConnection( m_pluginName, eventPtr->id() );
    }
    void handleConnectionClosed( const std::string /*handler*/&, CommonHttp::EventPtr eventPtr ) {
        if( !eventPtr )
            throw std::logic_error( "Null-pointer exception" );

        const char* eventName = CommonHttp::Resources::HttpEvents::HttpConnectionClosed;
        std::unique_lock< std::mutex > lock( m_mutex );
        m_events.push_back( { eventName, eventPtr->toJson() } );
        lock.unlock();

        m_condVar.notify_all();
    }

    using EventHandler = std::function< void( const std::string&, CommonHttp::EventPtr ) >;
    EventHandler makeDataReceivedAndClose() {
        return [this]( const std::string& name, CommonHttp::EventPtr eventPtr ) {
            handleDataReceivedAndClose( name, std::move( eventPtr ) );
        };
    }
    EventHandler makeConnectionClosed() {
        return [this]( const std::string& name, CommonHttp::EventPtr eventPtr ) {
            handleConnectionClosed( name, std::move( eventPtr ) );
        };
    }

    using EventPair = std::pair< std::string, Json >;
    using Events = std::deque< EventPair >;
    const EventPair* events( size_t index ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        return &m_events.at( index );
    }

    EventPair waitForEvent( int timeoutSec, const std::string& message, const char* file, int line ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        if( !m_condVar.wait_for( lock, std::chrono::seconds( timeoutSec ), [this] { return !m_events.empty(); } ) ) {
            ADD_FAILURE_AT( file, line ) << "No http events received. " << message;
            throw std::exception();
        }
        auto tmp = m_events.front();
        m_events.pop_front();
        return tmp;
    }

private:
    CommonHttp::Http& m_http;
    std::string m_pluginName;

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condVar;

    Events m_events;
};

}  // namespace Tests
}  // namespace HubPlatform