#include "uid/extensions/http_links/http_links_manager.h"
#include "uid/extensions/http_links/connection_info.h"
#include "uid/extensions/http_links/errors.h"
#include "uid/extensions/http_links/events.h"
#include "uid/extensions/http_links/resources.h"

#include "uid/ui_service.h"
#include "service_interfaces/service.h"
#include "database_client/idbclient.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include <utility>

namespace HubPlatform {
namespace HttpLinks {

Json toJsonDb( const HttpLink& link ) {
    namespace Fields = Resources::HttpLinkFields;
    return JSON( db::ID_FIELD_NAME << link.linkId() << Fields::handler << link.handler() << Fields::userData
                                   << link.userData() << Fields::timeout << link.timeout() << Fields::answer
                                   << link.answer() << Fields::url << link.insecureLink() << Fields::secureUrl
                                   << link.secureLink() << Fields::pluginId << link.pluginId() << Fields::deleted
                                   << link.deleted() );
}

bool HttpLinksManager::fromJsonDb( HttpLink& link, const Json& json ) {
    namespace Fields = Resources::HttpLinkFields;
    link.handler( json_get_str( json, Fields::handler ) );
    link.linkId( json_get_str( json, Fields::linkId ) );
    {
        Json userData;
        if( json_get( json, Fields::userData, userData ) )
            link.userData( std::move( userData ) );
    }
    link.timeout( json_get_int( json, Fields::timeout, Resources::Limits::RESPONSE_TIMEOUT_DEFAULT ) );
    link.answer( json_get_bool( json, Fields::answer, false ) );
    link.insecureLink( json_get_str( json, Fields::url ) );
    link.secureLink( json_get_str( json, Fields::secureUrl ) );
    link.pluginId( json_get_str( json, Fields::pluginId ) );
    link.deleted( json_get_int( json, Fields::deleted, 0 ) );

    return true;
}

HttpLinksManager::HttpLinksManager( Services::Service& hostService, HttpLinksEventHandler& handler )
: m_hostService( hostService )
, m_eventHandler( handler ) {}

void HttpLinksManager::setUp() {
    Network::RemoteInterface networkInterface{ m_hostService };
    auto activeInterface = networkInterface.getActiveNetworkInterface();
    if( !activeInterface.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "HttpLinksManager::setUp: activeInterface: " ) + cc::j( activeInterface ) );
        m_interface = json_get_str( activeInterface[ "ipv4" ], "ip" );
    }

    if( auto* uiService = dynamic_cast< Uid::Service* >( &m_hostService ) ) {
        m_securePort = uiService->getServerPort();
        m_insecurePort = uiService->getInsecureServerPort();
        LOGGER_DEBUG_RAW( cc::debug( "HttpLinksManager::setUp: secure port=" ) + cc::num10( m_securePort ) +
                          cc::debug( ", insecure port=" ) + cc::num10( m_insecurePort ) );
    }

    m_cleanUpTask = m_hostService.createPostponedTask( [this]( Services::PostponedTask& ) {
        clearClosedConnections();
        clearDeletedLinks();
    } );
    m_cleanUpTask->start( std::chrono::seconds( 30 ),
                          std::chrono::seconds( Resources::Limits::HttpLinksConnectionsCleanUpIntervalSec ) );

    const char* isComponentTestRunning = std::getenv( "RUNNING_COMPONENT_TESTS" );
    if( !isComponentTestRunning ) {
        m_httpLinkUpdateTask =
            m_hostService.createPostponedTask( [this]( Services::PostponedTask& ) { checkPluginsSecureLink(); } );
        m_httpLinkUpdateTask->start( std::chrono::seconds( 120 ), false );
    }
}

HttpLinksManager::~HttpLinksManager() {
    if( m_cleanUpTask ) {
        m_cleanUpTask->stop();
        m_cleanUpTask.reset();
    }
    if( m_httpLinkUpdateTask ) {
        m_httpLinkUpdateTask->stop();
        m_httpLinkUpdateTask.reset();
    }
}

HttpLinksManager::LinkId HttpLinksManager::createHttpLink( HttpLink linkParams ) {
    Json found;
    if( linkParams.pluginId().empty() ) {
        throw HttpLinksError( "pluginId required" );
    }
    if( !linkParams.linkId().empty() ) {
        found = getDBClient().findOne( DbNames::HttpLinks, JSON( db::ID_FIELD_NAME << linkParams.linkId() ), nullptr );
        if( !found.empty() ) {
            throw HttpLinksError( "Link with this id already exists" );
        }
    }
    else {
        linkParams.linkId( db::DbId::genOID() );
    }

    if( !found.empty() ) {
        throw HttpLinksError( "Link with this id already exists" );
    }

    LOGGER_TRACE( "pluginId: " + linkParams.pluginId() + ", linkId: " + linkParams.linkId() );

    auto url = makeInsecureLink( linkParams.pluginId(), linkParams.linkId() );
    linkParams.insecureLink( url );

    // TODO: only if the service has a secure interface: field is mandatory, but can be empty
    auto secureUrl = makeSecureLink( linkParams.pluginId(), linkParams.linkId() );
    linkParams.secureLink( secureUrl );

    getDBClient().insert( DbNames::HttpLinks, toJsonDb( linkParams ) );

    return linkParams.linkId();
}

bool HttpLinksManager::deleteHttpLink( const std::string& pluginId, const LinkId& id ) {
    namespace Fields = Resources::HttpLinkFields;

    auto found = getDBClient().findOne( DbNames::HttpLinks, JSON( db::ID_FIELD_NAME << id ), nullptr );
    if( found.empty() ) {
        LOGGER_TRACE( cc::trace( "Record not found, pluginId: " ) + pluginId + cc::trace( ", linkId: " ) + id );
        return false;
    }
    if( found.count( Fields::pluginId ) ) {
        if( json_get_str( found, Fields::pluginId ) != pluginId ) {
            throw HttpLinksError( "Can delete only own http links" );
        }

        getDBClient().update( DbNames::HttpLinks,
                              JSON( db::ID_FIELD_NAME << id ),
                              JSON( "$set" << JSON( Fields::deleted << time( nullptr ) ) ) );

        using namespace Resources;
        // close all connections before deleting
        Json fieldsToReturn = JSON( ConnectionDbFields::id );
        JsonArray connections = getDBClient().findAll(
            DbNames::TempHttpLinksConnections,
            JSON( db::ID_FIELD_NAME << ConnectionDbFields::linkId << ConnectionDbFields::closed << 0 ),
            &fieldsToReturn );
        for( const auto& conn : connections ) {
            auto connId = json_get_str( conn, ConnectionDbFields::id );
            if( !connId.empty() ) {
                closeConnection( pluginId, connId );
            }
        }

        return true;
    }
    return false;
}

Json HttpLinksManager::getLinkInfoRaw( const HttpLinksManager::LinkId& linkId ) const {
    return getDBClient().findOne( DbNames::HttpLinks, JSON( db::ID_FIELD_NAME << linkId ), nullptr );
}

HttpLinksManager::LinkInfo HttpLinksManager::getLinkInfo( const PluginId& pluginId, const LinkId& linkId ) const
    noexcept {
    namespace Fields = Resources::HttpLinkFields;

    Json found;
    HttpLinksManager::LinkInfo info;
    try {
        found = getLinkInfoRaw( linkId );

        if( found.empty() )
            throw HttpLinksError( "Http Link does not exist" );
        if( json_get_str( found, Fields::pluginId ) != pluginId )
            throw HttpLinksError( "Can fetch only own http links" );
        if( json_get_int( found, Fields::deleted, 0 ) > 0 )
            throw HttpLinksError( "Http Link is deleted" );

        jsonCopy( found, Fields::userData, info );
        jsonCopy( found, Fields::url, info );
        jsonCopy( found, Fields::secureUrl, info );
        info[ Fields::linkId ] = found[ db::ID_FIELD_NAME ];
        jsonCopy( found, Fields::answer, info );
        auto connectionIds = Json::array();

        auto fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 );
        JsonArray connections = getDBClient().findAll(
            DbNames::TempHttpLinksConnections, JSON( "linkId" << linkId << "closed" << 0 ), &fieldsToReturn );
        if( !connections.empty() ) {
            for( const auto& item : connections ) {
                connectionIds.push_back( json_get_str( item, db::ID_FIELD_NAME ) );
            }
        }
        info[ "connections" ] = connectionIds;
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_WARN( cc::error( "Exception: " ) + cc::note( ex.what() ) );
    }
    return info;
}

std::string HttpLinksManager::makeSecureLink( const PluginId& pluginId, const LinkId& linkId ) const {
    return std::string{ "https://" } + m_interface + ":" + std::to_string( m_securePort ) +
           Resources::linksPluginsEndpointPath + "/" + pluginId + "/" + linkId;
}

std::string HttpLinksManager::makeInsecureLink( const PluginId& pluginId, const LinkId& linkId ) const {
    return std::string{ "http://" } + m_interface + ":" + std::to_string( m_insecurePort ) +
           Resources::linksPluginsEndpointPath + "/" + pluginId + "/" + linkId;
}

bool HttpLinksManager::closeConnection( const PluginId& pluginId, const ConnectionId& connectionId ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        auto& connInfo = foundIt->second;
        connInfo.forceClose( true );
        emitWritable( connInfo );
        return true;
    }

    return false;
}

/** @brief Append data to a session and notify the endpoint that there are more data to flush.
 * @param payload object of a kind { id: string, data: string|binary, last: ?bool, code: ?int, code_status: ?string } */
bool HttpLinksManager::sendData( const PluginId& pluginId, const ConnectionId& connectionId, const Payload& payload ) {
    if( connectionId.empty() || payload.count( "data" ) == 0 )
        throw HttpLinksError( "Bad parameters: id or data should not be empty" );

    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        auto& connInfo = foundIt->second;
        bool isLast = json_get_bool( payload, "last", false );
        const Json& jData = payload[ "data" ];
        std::string data = json_get_str( jData );

        int code = json_get_int( payload, "code", -1 );
        auto codeStatus = json_get_str( payload, "code_status" );
        if( -1 != code )
            connInfo.exitCode( code, codeStatus );
        if( payload.count( "headers" ) ) {
            ConnectionInfo::Headers headers;
            ConnectionInfo::fetchArray( payload, "headers", headers );
            connInfo.headers( headers );
        }

        connInfo.appendData( std::move( data ), isLast );

        if( !isLast )
            connInfo.emitNextDataSend( true );
        emitWritable( connInfo );
        return true;
    }
    return false;
}

HttpLinksManager::ConnectionId HttpLinksManager::createConnection( const LinkId& linkId,
                                                                   ConnectionInfo connectionInfo,
                                                                   const void* wsiPtr ) {
    if( linkId.empty() )
        throw HttpLinksError( "Empty link id" );
    connectionInfo.id( db::DbId::genOID() );
    connectionInfo.linkId( linkId );

    LockGuard lock( m_mutexConnections );
    m_connections.emplace( connectionInfo.id(), ConnectionHandle( linkId, wsiPtr ) );
    lock.unlock();

    getDBClient().insert( DbNames::TempHttpLinksConnections, connectionInfo.toJsonDb() );

    return connectionInfo.id();
}

/** Marks the connection as closed by lws */
bool HttpLinksManager::markConnectionAsClosed( const ConnectionId& id ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( id );
    if( foundIt == m_connections.end() )
        return false;

    m_connections.erase( foundIt );

    auto found = getDBClient().findOne( DbNames::TempHttpLinksConnections, JSON( db::ID_FIELD_NAME << id ), nullptr );
    if( found.empty() )
        return false;
    getDBClient().update( DbNames::TempHttpLinksConnections,
                          JSON( db::ID_FIELD_NAME << id ),
                          JSON( "$set" << JSON( "closed" << time( nullptr ) ) ) );

    return true;
}

bool HttpLinksManager::getConnectionInfo( const std::string& id, ConnectionInfo& info ) const {
    auto found = getDBClient().findOne( DbNames::TempHttpLinksConnections, JSON( db::ID_FIELD_NAME << id ), nullptr );
    if( found.empty() )
        return false;
    info.clear();
    info.fromJsonDb( found );

    return true;
}

void HttpLinksManager::setHostOptions( std::string interface, uint16_t insecurePort, uint16_t securePort ) {
    m_interface = std::move( interface );
    setHostPorts( insecurePort, securePort );
}

void HttpLinksManager::setHostPorts( uint16_t insecurePort, uint16_t securePort ) {
    m_insecurePort = insecurePort;
    m_securePort = securePort;
}

IDbClient& HttpLinksManager::getDBClient() {
    return m_hostService.getDBClient();
}

const IDbClient& HttpLinksManager::getDBClient() const {
    return m_hostService.getDBClient();
}

/** Obtains record like: {"interfaceId":"eth0","ipv4":"192.168.43.46","type":"ethernet"} or
 * if Internet is down: {"interfaceId":"","ipv4":"","type":"ethernet"} */
void HttpLinksManager::updateCurrentIp( const JsonObject& event ) {
    auto networkType = json_get_str( event, "type" );
    if( networkType.empty() ) {
        Log::error( cc::error( "Empty network type" ) );
        return;
    }
    if( networkType == "modem" ) {
        Log::warn( cc::warn( "Ignore modem network type" ) );
        return;
    }
    auto newIpAddress = json_get_str( event, "ipv4" );
    if( newIpAddress.empty() ) {
        Log::error( cc::error( "No network ip address" ) );
        return;
    }
    if( m_interface == newIpAddress ) {
        return;
    }

    LOGGER_INFO( cc::info( "Ip address is updated: " ) + cc::j( event ) );
    m_interface = newIpAddress;

    updatePluginsHttpLinks();
}

HttpLinksEventHandler& HttpLinksManager::eventHandler() {
    return m_eventHandler;
}

void HttpLinksManager::clearClosedConnections() {
    const int safeIntervalSec = Resources::Limits::HttpLinksConnectionsLifetimeAfterClosedSec;
    auto deleteBefore = time( nullptr ) - safeIntervalSec;
    auto query = JSON( "created" << JSON( "$gt" << 0 ) << "closed" << JSON( "$gt" << 0 << "$lt" << deleteBefore ) );

    auto recordsBefore = getDBClient().count( DbNames::TempHttpLinksConnections, Json::object() );
    getDBClient().remove( DbNames::TempHttpLinksConnections, query );
    auto recordsAfter = getDBClient().count( DbNames::TempHttpLinksConnections, Json::object() );
    if( recordsBefore != recordsAfter ) {
        LOGGER_DEBUG_RAW( cc::debug( "HttpLinks: closed connections cleared: " ) +
                          cc::num10( (int) recordsBefore - (int) recordsAfter ) );
    }
}

void HttpLinksManager::clearDeletedLinks() {
    const int safeIntervalSec = Resources::Limits::HttpLinksLifetimeAfterDeletedSec;
    auto deleteBefore = time( nullptr ) - safeIntervalSec;
    auto query = JSON( Resources::HttpLinkFields::deleted << JSON( "$gt" << 0 << "$lt" << deleteBefore ) );

    auto recordsBefore = getDBClient().count( DbNames::HttpLinks, Json::object() );
    getDBClient().remove( DbNames::HttpLinks, query );
    auto recordsAfter = getDBClient().count( DbNames::HttpLinks, Json::object() );
    if( recordsBefore != recordsAfter ) {
        LOGGER_DEBUG_RAW( cc::debug( "HttpLinks: deleted links cleared: " ) +
                          cc::num10( (int) recordsBefore - (int) recordsAfter ) );
    }
}

bool HttpLinksManager::connectionForceClosed( const ConnectionId& connectionId ) const {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        const auto& connInfo = foundIt->second;
        return connInfo.forceClose();
    }
    return false;
}

bool HttpLinksManager::fetchNextPortion( const std::string& connectionId, std::vector< char >& buffer ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        auto& connInfo = foundIt->second;
        size_t totalSize = 0;
        while( connInfo.portions() > 0 ) {
            std::string& portion = connInfo.frontPortion();
            buffer.insert( buffer.end(), portion.data(), portion.data() + portion.size() );
            totalSize += portion.size();

            connInfo.popFrontPortion();

            if( totalSize > 0 ) {
                break;
            }
        }
        return totalSize > 0 || !connInfo.last();
    }
    return false;
}

bool HttpLinksManager::isLast( const ConnectionId& connectionId ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        const auto& connInfo = foundIt->second;
        return connInfo.last();
    }
    return false;
}

bool HttpLinksManager::isFirst( const HttpLinksManager::ConnectionId& connectionId ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        const auto& connInfo = foundIt->second;
        return connInfo.first();
    }
    return false;
}

void HttpLinksManager::emitLinkUpdated( EventBase& event ) {
    eventHandler().onHttpLinkUpdated( event );
}

bool HttpLinksManager::getExitCodes( const ConnectionId& connectionId, ExitCodes& result ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        const auto& connInfo = foundIt->second;
        result.code = connInfo.code();
        result.codeStatus = connInfo.codeStatus();
        if( !connInfo.headers().empty() )
            result.headers = connInfo.headers();
        return !result.empty();
    }
    return false;
}

void HttpLinksManager::dataSendEventRequested( const std::string& connectionId, bool value ) {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        auto& connInfo = foundIt->second;
        connInfo.emitNextDataSend( value );
    }
}

bool HttpLinksManager::dataSendEventRequested( const std::string& connectionId ) const {
    LockGuard lock( m_mutexConnections );
    auto foundIt = m_connections.find( connectionId );
    if( foundIt != m_connections.end() ) {
        const auto& connInfo = foundIt->second;
        return connInfo.emitNextDataSend();
    }
    return false;
}

void HttpLinksManager::emitWritable( const ConnectionHandle& handle ) {
    if( handle.lws() ) {
        // can be null only in tests
        lws_callback_on_writable( const_cast< lws* >( static_cast< const lws* >( handle.lws() ) ) );
    }
}

void HttpLinksManager::checkPluginsSecureLink() {
    LOGGER_INFO_RAW( cc::info( "HttpLinks: checking plugins secure link started" ) );
    namespace Fields = Resources::HttpLinkFields;
    JsonArray httpLinkRecords = getDBClient().findAll( DbNames::HttpLinks, JSON( Fields::deleted << 0 ), nullptr );
    LOGGER_DEBUG_RAW( cc::debug( "HttpLinks: records found: " ) + cc::j( httpLinkRecords ) );
    for( const auto& linkInfo : httpLinkRecords ) {
        auto secureLink = json_get_str( linkInfo, Fields::secureUrl );
        if( secureLink.find( m_interface ) == std::string::npos ) {
            LOGGER_INFO_RAW( cc::info( "HttpLinks: Link updating: " ) + cc::note( secureLink ) );
            updatePluginsHttpLinks();
            break;
        }
    }
    LOGGER_INFO_RAW( cc::info( "HttpLinks: checking plugins secure link ended" ) );
}

void HttpLinksManager::updatePluginsHttpLinks() {
    namespace Fields = Resources::HttpLinkFields;
    Json fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 << Fields::pluginId << 1 << Fields::userData << 1 );
    JsonArray links = getDBClient().findAll( DbNames::HttpLinks, JSON( Fields::deleted << 0 ), &fieldsToReturn );
    for( const auto& linkInfo : links ) {
        auto linkId = json_get_str( linkInfo, db::ID_FIELD_NAME );
        auto pluginId = json_get_str( linkInfo, Fields::pluginId );
        Json userData = Json::object();
        json_get( linkInfo, Fields::userData, userData );

        // update links in db
        auto insecureUrl = makeInsecureLink( pluginId, linkId );
        auto secureUrl = makeSecureLink( pluginId, linkId );
        getDBClient().update( DbNames::HttpLinks,
                              JSON( db::ID_FIELD_NAME << linkId ),
                              JSON( "$set" << JSON( Fields::url << insecureUrl << Fields::secureUrl << secureUrl ) ) );

        // send linkUpdatedEvent 'http_link_updated' for every link of every plugin
        LinkUpdatedEvent linkUpdatedEvent( linkId, userData, insecureUrl, secureUrl );
        emitLinkUpdated( linkUpdatedEvent );
        LOGGER_INFO_RAW( cc::info( "Http link is updated: " ) + cc::note( linkId ) );
    }
}

}  // namespace HttpLinks
}  // namespace HubPlatform