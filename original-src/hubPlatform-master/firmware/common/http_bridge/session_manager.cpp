#include "session_manager.h"
#include "session.h"
#include "resources.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace HttpBridge {

SessionPtr SessionManager::startSession( lws* wsi, const char* type ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    Session::SessionType sessionType = Session::SessionType::HTTP;
    if( type && 0 == strcmp( type, Resources::sessionTypeEventStream ) ) {
        sessionType = Session::SessionType::EVENT_STREAM;
        m_eventStreams++;
    }

    auto ptr = std::make_shared< Session >( sessionType );
    if( ptr == nullptr )
        throw std::runtime_error( "Could not allocate session object" );
    ptr->init( wsi );

    auto found = m_sessions.find( wsi );
    if( found != m_sessions.end() ) {
        found->second = ptr;
    }
    else {
        m_sessions.emplace( wsi, ptr );
        LOGGER_TRACE_RAW( "Start new HTTP session. Sessions: " + cc::num10( m_sessions.size() ) );
    }

    return ptr;
}

void SessionManager::finishSession( lws* wsi ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    auto foundIt = m_sessions.find( wsi );
    if( foundIt != m_sessions.end() ) {
        SessionPtr ptr = foundIt->second;
        if( ptr ) {
            if( ptr->getData() )
                ptr->getData()->finished( true );
            if( ptr->sessionType() == Session::SessionType::EVENT_STREAM )
                m_eventStreams--;
        }
        m_sessions.erase( foundIt );
        LOGGER_TRACE_RAW( "Finish HTTP session. Sessions: " + cc::num10( m_sessions.size() ) );
    }
}

SessionPtr SessionManager::findSession( lws* wsi ) const {
    std::unique_lock< std::mutex > lock( m_mutex );

    auto foundIt = m_sessions.find( wsi );
    if( foundIt != m_sessions.end() ) {
        return foundIt->second;
    }
    return nullptr;
}

void SessionManager::forEachSession( const std::string& msg,
                                     std::function< void( const std::string&, SessionPtr ) > callback ) {
    if( callback == nullptr )
        return;

    std::unique_lock< std::mutex > lock( m_mutex );
    for( auto& sessionIt : m_sessions ) {
        try {
            callback( msg, sessionIt.second );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Failed to execute callback: " ) + cc::note( ex.what() ) );
        }
    }
}

size_t SessionManager::sessionCount() const {
    std::unique_lock< std::mutex > lock( m_mutex );
    return m_sessions.size() + m_externalSessions;
}

size_t SessionManager::eventStreamsCount() const {
    std::unique_lock< std::mutex > lock( m_mutex );
    return m_eventStreams;
}

void SessionManager::incrementExternalSessions() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_externalSessions++;
}

void SessionManager::decrementExternalSessions() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_externalSessions--;
}

size_t SessionManager::externalSessions() const {
    std::unique_lock< std::mutex > lock( m_mutex );
    return m_externalSessions;
}

}  // namespace HttpBridge
}  // namespace HubPlatform