#include "http/http.h"
#include "http/http_impl.h"
#include "http/http_error.h"
#include "database_client/dbid.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <future>

namespace HubPlatform {
namespace CommonHttp {

namespace {

class ScopeGuard {
public:
    explicit ScopeGuard( std::function< void() > action )
    : m_action( std::move( action ) ) {}
    ~ScopeGuard() {
        try {
            m_action();
        }
        catch( ... ) {
        }
    }

private:
    std::function< void() > m_action;
};

}  // namespace

HttpImpl::HttpImpl( HttpEventEmitter& emitter )
: m_eventEmitter( emitter )
, m_http( *this, m_connectionQueue ) {}

HttpImpl::~HttpImpl() {
    stop();

    {  // do not remove this scope!
        UniqueLock lock( m_pluginsMutex );
        clearPlugins();
    }
    m_http.reset();
}

void HttpImpl::clearPlugins() {
    for( auto pluginIt : m_plugins ) {
        for( const auto& connIt : pluginIt.second ) {
            if( ConnectionPtr ptr = connIt.second ) {
                auto& connection = *ptr;
                try {
                    m_http.removeMultiHandle( connection );
                }
                catch( const HttpError& ex ) {
                    LOGGER_ERROR( cc::error( "Could not add to connection 'remove' queue: " ) +
                                  cc::note( connection.getId() ) );
                }
                connection.flushBuffer( false );
                connection.emitConnectionClosed( static_cast< int >( Reason::NONE ), "Ok" );
            }
        }

        // FIXME: clears connections here, while multi-handle has links on them.
        pluginIt.second.clear();
    }
    m_plugins.clear();
}

std::string HttpImpl::generateId() {
    return db::DbId::genOID();
}

ConnectionPtr HttpImpl::makeConnection( const HttpOptions& opts ) {
    auto id = generateId();
    auto ptr = std::make_shared< Connection >( opts, m_eventEmitter );
    ptr->setId( id );

    return ptr;
}

/** Synchronized call: write lock (m_plugins) */
bool HttpImpl::addConnection( const PluginId& pluginId, const ConnectionId& id, ConnectionPtr pConnection ) {
    if( pluginId.empty() || id.empty() || pConnection == nullptr )
        return false;

    if( pConnection && pConnection->getPluginId().empty() )
        pConnection->setPluginId( pluginId );

    {
        UniqueLock lock( m_pluginsMutex );
        auto pluginIt = m_plugins.find( pluginId );
        if( pluginIt == m_plugins.end() ) {
            ConnectionMap list;
            list.emplace( id, pConnection );
            m_plugins.emplace( pluginId, std::move( list ) );
        }
        else {
            auto& connections = pluginIt->second;
            if( connections.size() >= getMaxPluginConnections() )
                throw HttpError( Resources::HTTP_ERROR_CONNECTION_LIMIT_REACHED );
            connections.emplace( id, pConnection );
        }
    }

    m_connectionQueue.askAddToMultiHandle( pluginId, pConnection->getId() );

    UniqueLock lock( m_pluginsMutex );
    if( !m_isRunning ) {
        if( joinable() ) {
            m_thread.join();
            m_isJoinable = false;
        }
        start();
        std::this_thread::sleep_for( std::chrono::milliseconds{ 200 } );
    }
    return true;
}

/** Synchronized call: read lock (m_plugins) */
ConnectionPtr HttpImpl::findConnection( const PluginId& pluginId, const ConnectionId& id ) {
    SharedLock lock( m_pluginsMutex );
    auto pluginIt = m_plugins.find( pluginId );
    if( pluginIt != m_plugins.end() ) {
        auto& connections = pluginIt->second;
        auto foundIt = connections.find( id );
        if( foundIt != end( connections ) )
            return foundIt->second;
    }
    return nullptr;
}

ConnectionPtr HttpImpl::findByHandle( CURL* curl ) const {
    if( nullptr == curl )
        return nullptr;

    SharedLock lock( m_pluginsMutex );
    for( const auto& plugin : m_plugins ) {
        for( const auto& connection : plugin.second ) {
            if( connection.second->getCURL() == curl ) {
                return connection.second;
            }
        }
    }
    return nullptr;
}

namespace {
Reason makeHttpReason( int httpStatus ) {
    auto reasonCode = Reason::NONE;

    if( httpStatus >= 400 && httpStatus < 600 ) {
        reasonCode = Reason::CLOSED_BY_SERVER;
    }
    return reasonCode;
}
}  // namespace

/** Synchronized call: write lock (m_plugins)
 *  Throws HttpError if connection doesn't exist [if enabled].
 * */
bool HttpImpl::closeConnection( const PluginId& pluginId,
                                const ConnectionId& connId,
                                bool removeFromMulti,
                                bool throwIfConnectionIdInvalid ) {
    LOGGER_DEBUG_RAW( cc::debug( "Close HTTP connection, id: " ) + connId );
    ConnectionPtr ptr = popConnection( pluginId, connId );

    if( !ptr.get() ) {
        if( throwIfConnectionIdInvalid ) {
            throw HttpError( Resources::HTTP_ERROR_CONNECTION_DOESNT_EXIST );
        }
        else {
            return false;
        }
    }

    if( removeFromMulti ) {
        m_connectionQueue.askRemoveFromMultiHandle( ptr->getPluginId(), ptr->getId() );
        m_connectionQueue.markForRemoval( ptr );
    }

    auto reasonCode = makeHttpReason( ptr->response().getHttpStatus() );
    const char* reasonMessage = ConnectionClosedEvent::reasonName( reasonCode );

    // Check that for the connections inMulty() == true the connectionClosed emitted only once!
    ptr->emitConnectionClosed( toInt( reasonCode ), reasonMessage );
    return removeFromMulti ? true : false;
}

/** Synchronized call: read lock (m_plugins) */
void HttpImpl::closePluginConnections( const PluginId& pluginId ) {
    std::vector< std::string > connectionIds;

    {
        SharedLock lock( m_pluginsMutex );
        auto pluginIt = m_plugins.find( pluginId );
        if( pluginIt == m_plugins.end() ) {
            return;
        }

        for( const auto& connection : pluginIt->second ) {
            connectionIds.push_back( connection.first );
        }
    }
    if( !connectionIds.empty() )
        LOGGER_INFO( cc::info( "Closing all plugin connections: " ) + cc::num10( connectionIds.size() ) );

    if( !connectionIds.empty() ) {
        LOGGER_INFO( cc::info( "Close plugin HTTP connections: " ) + cc::num10( connectionIds.size() ) );
    }
    for( const auto& connectionId : connectionIds ) {
        try {
            closeConnection( pluginId, connectionId, true, false );
        }
        catch( const HttpError& ex ) {
            LOGGER_ERROR( cc::error( "Error closing plugin '" ) + cc::attention( pluginId ) +
                          cc::error( "' connections: " ) + cc::note( ex.what() ) );
        }
    }
}

void HttpImpl::setMaxPluginConnections( unsigned value ) {
    m_maxPluginConnections = value;
}

unsigned HttpImpl::getMaxPluginConnections() const {
    return m_maxPluginConnections;
}

void HttpImpl::runMultiHandleThread( std::shared_ptr< std::promise< void > > readyToRun ) {
    ScopeGuard guard( [this]() { m_isRunning = false; } );

    m_http.runMultiHandleThread( std::move( readyToRun ) );
}

void HttpImpl::start() {
    if( !m_isRunning && !m_isJoinable ) {
        m_isJoinable = true;
        m_isRunning = true;
        m_isStopped = false;

        auto promisePtr = std::make_shared< std::promise< void > >();
        auto future = promisePtr->get_future();

        auto startThread = [this, promisePtr]() { this->runMultiHandleThread( promisePtr ); };

        m_thread = std::thread( std::move( startThread ) );
        const std::chrono::seconds commandTimeout( 10 );
        auto status = future.wait_for( commandTimeout );
        if( status == std::future_status::ready ) {
            m_isJoinable = true;
            m_isRunning = true;
            m_isStopped = false;
        }
        else {
            LOGGER_ERROR( "Could not http worker start thread in 10 seconds" );
            if( m_thread.joinable() )
                m_thread.detach();
            m_isJoinable = false;
            m_isRunning = false;
            m_isStopped = false;
        }
    }
}

bool HttpImpl::joinable() {
    if( !m_isJoinable )
        return m_thread.joinable();

    // thread requires some time to be joinable
    for( size_t i = 0; i < 50; ++i ) {
        if( m_thread.joinable() )
            return true;
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    }

    LOGGER_WARN_RAW( cc::warn( "HttpImpl::joinable: thread hanged and can't be joined" ) );
    return false;
}

void HttpImpl::join() {
    if( joinable() )
        m_thread.join();
    clearPlugins();
    m_isJoinable = false;
}

void HttpImpl::stop() {
    // stop running thread
    m_isStopped = true;

    if( joinable() ) {
        join();
    }
    m_isRunning = false;
}

bool HttpImpl::isRunning() {
    return m_isRunning;
}

bool HttpImpl::askAddToMultiHandle( const HttpImpl::PluginId& pluginId, const ConnectionId& connId ) {
    m_connectionQueue.askRemoveFromMultiHandle( pluginId, connId );
    return true;
}

/** Companion of closeConnection() method: pops connection from locked by a mutex part of an object and returns it.
 *  Returns empty pointer if nothing was found.
 */
ConnectionPtr HttpImpl::popConnection( const HttpImpl::PluginId& pluginId, const ConnectionId& connId ) {
    ConnectionPtr ptr;

    UniqueLock lock( m_pluginsMutex );

    auto pluginIt = m_plugins.find( pluginId );
    if( pluginIt != m_plugins.end() ) {
        auto& connections = pluginIt->second;
        auto foundIt = connections.find( connId );
        if( foundIt != end( connections ) ) {
            ptr = foundIt->second;

            connections.erase( foundIt );
            if( connections.empty() )
                m_plugins.erase( pluginId );
        }
    }
    return ptr;
}

bool HttpImpl::isStopped() const {
    return m_isStopped;
}

HttpMultiClient& HttpImpl::getClient() {
    return m_http;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
