#pragma once

#include "uid/extensions/ui_extension.h"
#include "uid/extensions/ui_bus_extension.h"
#include "uid/extensions/ui_http_extension.h"
#include "uid/extensions/http_links/http_link.h"
#include "uid/extensions/http_links/connection_info.h"
#include "uid/extensions/http_links/event_handler.h"
#include "uid/extensions/http_links/connection_handle.h"
#include "service_interfaces/postponed_task.h"
#include "database_client/db_tables_names.h"
#include "bus/messagesclasses.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <set>
#include <utility>

class IDbClient;

namespace HubPlatform {
namespace Services {
class Service;
}

namespace HttpLinks {

namespace DbNames {
constexpr const char* HttpLinks = UiExtensionsHttpLinksDBName;
constexpr const char* TempHttpLinksConnections = TempUiExtensionsHttpLinksConnectionsDBName;
}  // namespace DbNames

struct ExitCodes {
    int code{ 0 };
    std::string codeStatus;
    ConnectionInfo::Headers headers;

public:
    bool empty() const {
        return code == 0 && headers.empty();
    }
};

/** Handles persistent http links collection and operations. */
class HttpLinksManager {
public:
    using PluginId = std::string;
    using LinkId = std::string;
    using ConnectionId = std::string;
    using Payload = Json;
    using LinkInfo = Json;

public:
    explicit HttpLinksManager( Services::Service& hostService, HttpLinksEventHandler& handler );
    ~HttpLinksManager();

    void setUp();

    // lua api functions
    LinkId createHttpLink( HttpLink linkParams );
    bool deleteHttpLink( const PluginId& pluginId, const LinkId& id );
    LinkInfo getLinkInfo( const PluginId& pluginId, const LinkId& linkId ) const noexcept;
    Json getLinkInfoRaw( const LinkId& linkId ) const;

    bool closeConnection( const PluginId& pluginId, const ConnectionId& connectionId );
    bool getConnectionInfo( const std::string& id, ConnectionInfo& info ) const;
    bool sendData( const PluginId& pluginId, const ConnectionId& connectionId, const Payload& payload );
    HttpLinksEventHandler& eventHandler();

    void clearClosedConnections();
    void clearDeletedLinks();

public:
    // service functions, called from http_server code
    void setHostOptions( std::string interface, uint16_t insecurePort, uint16_t securePort );
    void setHostPorts( uint16_t insecurePort, uint16_t securePort );
    ConnectionId createConnection( const LinkId& linkId, ConnectionInfo connectionInfo, const void* wsiPtr );
    bool markConnectionAsClosed( const ConnectionId& id );
    bool connectionForceClosed( const ConnectionId& id ) const;
    bool isLast( const ConnectionId& connectionId );
    bool isFirst( const ConnectionId& connectionId );
    bool fetchNextPortion( const ConnectionId& connectionId, std::vector< char >& buffer );
    bool getExitCodes( const ConnectionId& connectionId, ExitCodes& exitCodes );
    bool dataSendEventRequested( const std::string& connectionId ) const;
    void dataSendEventRequested( const std::string& connectionId, bool value );

    void updateCurrentIp( const JsonObject& json );

    bool fromJsonDb( HttpLink& link, const Json& json );

    std::string makeInsecureLink( const std::string& pluginId, const LinkId& linkId ) const;
    std::string makeSecureLink( const std::string& pluginId, const LinkId& linkId ) const;

private:
    IDbClient& getDBClient();
    const IDbClient& getDBClient() const;
    void emitLinkUpdated( EventBase& event );
    static void emitWritable( const ConnectionHandle& handle );
    void checkPluginsSecureLink();
    void updatePluginsHttpLinks();

private:
    Services::Service& m_hostService;
    HttpLinksEventHandler& m_eventHandler;

    std::string m_interface{ "127.0.0.1" };
    uint16_t m_insecurePort{ 17001 };
    uint16_t m_securePort{ 17000 };

    mutable std::recursive_mutex m_mutexConnections;
    using LockGuard = std::unique_lock< std::recursive_mutex >;
    using InMemoryConnections = std::map< ConnectionId, ConnectionHandle >;
    InMemoryConnections m_connections;

    Services::PostponedTaskPtr m_cleanUpTask{ nullptr };
    Services::PostponedTaskPtr m_httpLinkUpdateTask{ nullptr };
};

}  // namespace HttpLinks
}  // namespace HubPlatform