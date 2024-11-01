#include "http/http.h"
#include "http/http_impl.h"
#include "http/connection.h"
#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "database_client/dbid.h"

namespace HubPlatform {
namespace CommonHttp {

class Http::Impl : public HttpImpl {
public:
    explicit Impl( HttpEventEmitter& emitter )
    : HttpImpl( emitter ) {}
    ~Impl() override = default;
};

Http::Http()
: m_pimpl( new Impl( *this ) ) {
#ifdef _DEBUG
    this->verbose( true );
#endif
}

Http::~Http() {
    if( m_pimpl ) {
        if( m_pimpl->isRunning() )
            m_pimpl->stop();

        if( m_pimpl->joinable() )
            m_pimpl->join();
    }
}

void Http::checkImpl() {
    if( nullptr == m_pimpl )
        throw HttpError( "Invalid pimpl pointer" );
}

/** @brief Creates connection/request initialized by parameters passed.
 * Starts data transfer, raises http_data_received for/http_data_send events.
 *
 * @param options - various HTTP(s)-related options
 * @return connection id
 */
Http::RequestId Http::request( const PluginId& pluginId, const HttpOptions& options ) {
    checkImpl();

    ConnectionPtr conn = m_pimpl->makeConnection( options );
    if( conn == nullptr )
        throw HttpError( "Could not create connection object" );
    RequestId id = conn->getId();
    conn->setPluginId( pluginId );
    conn->prepare();

    LOGGER_DEBUG_RAW( cc::debug( "Http::request: connection id: " ) + cc::note( id ) );
    if( !m_pimpl->addConnection( pluginId, id, conn ) ) {
        Log::error( cc::error( "Http::request: Could not add new connection" ) );
        throw HttpError( "Could not add new connection object" );
    }

    return id;
}

void Http::repeatRequest( const PluginId& pluginId, const RequestId& requestId, const HttpOptions& options ) {
    checkImpl();

    ConnectionPtr ptr = m_pimpl->findConnection( pluginId, requestId );
    if( !ptr )
        throw HttpConnectionDoesntExistError( requestId );

    Log::info( "Http::repeatRequest: repeat request for connection: " + ptr->getId() );
    Connection& conn = *ptr.get();
    if( conn.inMulti() )
        throw HttpConnectionTransferPending( requestId );  // Sending of previous request data didn't finish

    conn.updateOptions( options );

    // initialize curl handle with new values
    conn.resetHandle();
    conn.prepare();

    // push it to queue
    m_pimpl->askAddToMultiHandle( pluginId, conn.getId() );
    if( !m_pimpl->joinable() ) {
        m_pimpl->start();
        std::this_thread::sleep_for( std::chrono::milliseconds{ 200 } );
    }
}

bool Http::closeConnection( const PluginId& pluginId, const Http::RequestId& requestId ) {
    if( m_pimpl )
        return m_pimpl->closeConnection( pluginId, requestId, true );
    return false;
}

void Http::closePluginConnections( const PluginId& pluginId ) {
    if( m_pimpl ) {
        m_pimpl->closePluginConnections( pluginId );
    }
}

/**
 * @return number of sent/buffered bytes
 */
int Http::sendData( const PluginId& pluginId, const RequestId& requestId, std::string data, bool last ) {
    checkImpl();

    auto connPtr = m_pimpl->findConnection( pluginId, requestId );
    if( !connPtr )
        throw HttpConnectionDoesntExistError( Resources::HTTP_ERROR_CONNECTION_DOESNT_EXIST );
    auto size = data.size();
    connPtr->nextPortion( std::move( data ), last );
    return size;
}

/** Throws HttpConnectionDoesntExistError if connection doesn't exist */
ConnectionInfo Http::getConnectionInfo( const PluginId& pluginId, const RequestId& requestId ) {
    checkImpl();

    auto ptr = m_pimpl->findConnection( pluginId, requestId );
    if( !ptr )
        throw HttpConnectionDoesntExistError( Resources::HTTP_ERROR_CONNECTION_DOESNT_EXIST );

    // id, url, user_data, max_data_length
    const HttpOptions& options = ptr->getOptions();
    ConnectionInfo info( requestId, options.url, options.userData, Limits::SEND_DATA_MAX_DATA_LENGTH );
    return info;
}

void Http::setMaxPluginConnections( unsigned value ) {
    if( m_pimpl )
        m_pimpl->setMaxPluginConnections( value );
}

unsigned Http::getMaxPluginConnections() const {
    if( m_pimpl )
        return m_pimpl->getMaxPluginConnections();
    return MAX_PLUGIN_CONNECTIONS;
}

bool Http::joinable() {
    if( m_pimpl )
        return m_pimpl->joinable();
    return false;
}

void Http::join() {
    if( m_pimpl && m_pimpl->joinable() )
        m_pimpl->join();
}

int Http::exitDelayInterval() const {
    if( m_pimpl )
        return m_pimpl->getClient().exitDelayInterval();
    return 0;
}

void Http::exitDelayInterval( int value ) {
    if( m_pimpl )
        m_pimpl->getClient().exitDelayInterval( value );
}

void Http::stop() {
    if( m_pimpl )
        m_pimpl->stop();
}

}  // namespace CommonHttp
}  // namespace HubPlatform
