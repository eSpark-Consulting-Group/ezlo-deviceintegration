#include "uid/handlers/hub_ssh_tunnel_commands_handler.h"
#include "uid/ezlo.h"

#include "base_utils/utils.h"

#include "system_utils/ssh.h"

#include <future>

namespace HubPlatform {
namespace Uid {

namespace {
#if defined( __HUB_TYPE_VHUB__ )
const int OwnSshPort = 50022;
#else
const int OwnSshPort = 22;
#endif
}  // namespace

HubSshTunnelCommandsHandler::HubSshTunnelCommandsHandler( Services::Service& hostService )
: m_service( hostService ) {}

void HubSshTunnelCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubSshTunnelCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "create", &HubSshTunnelCommandsHandler::handleCreateRequest },
        { "delete_all", &HubSshTunnelCommandsHandler::handleDeleteAllRequest },
    };

    const auto it = requestHandlers.find( method );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

// params: {
//        "serverAddress": "10.1.1.1",
//        "serverPort": 8822,
//        "serverUser": "root",
//        "serverPass": "pass",
//        "time": 60,
//        "tunnelPort": 22222
//    }
void HubSshTunnelCommandsHandler::handleCreateRequest( rpc::Message& rpcMessage ) {
    constexpr const int defaultServerPort = 8822;
    constexpr const int maxTimeMin = 60;
    constexpr const int defaultTimeMin = 60;

    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string serverAddress, serverUser, serverPass;
    int tunnelPort = 0;

    if( ( !getField( joParams, "serverAddress", joAnswer, serverAddress ) ) ||
        ( !getField( joParams, "serverUser", joAnswer, serverUser ) ) ||
        ( !getField( joParams, "tunnelPort", joAnswer, tunnelPort ) ) ||
        ( !getField( joParams, "serverPass", joAnswer, serverPass ) ) )
        return;

    const int serverPort = json_get_int( joParams, "serverPort", defaultServerPort );

    const int timeMin = std::min( maxTimeMin, json_get_int( joParams, "time", defaultTimeMin ) ) * 60;

    pid_t pid = -1;
    std::string errorMessage;
    try {
        struct Result {
            Process::Forker::Status status;
            int exitCode;
            std::string error;
        };

        auto promise = std::make_shared< std::promise< Result > >();
        Ssh::TunnelManager::Data data = { serverAddress, serverPort, serverUser, serverPass,
                                          tunnelPort,    OwnSshPort, timeMin };
        Ssh::TunnelManager tunnelManager;
        pid = tunnelManager.createSshTunnel(
            data, [promise]( pid_t, Process::Forker::Status status, int exitCode, const std::string& error ) {
                promise->set_value( { status, exitCode, error } );
            } );
        auto future = promise->get_future();
        auto futureStatus = future.wait_for( std::chrono::seconds( 1 ) );
        if( futureStatus == std::future_status::ready ) {
            auto result = future.get();
            std::string reason;
            if( result.status == Process::Forker::Status::Ok && result.exitCode == 5 ) {
                reason = "ssh tunnel was rejected by cloud";
            }
            else if( result.status == Process::Forker::Status::Ok && result.exitCode == 255 ) {
                reason = "ssh tunnel was closed by cloud";
            }
            else {
                reason = result.error;
            }
            errorMessage = "status: " + std::to_string( (int) result.status ) +
                           ", code: " + std::to_string( result.exitCode ) + ", reason: " + reason;
        }
    }
    catch( const std::exception& ex ) {
        errorMessage = ex.what();
    }

    if( pid <= 0 || !errorMessage.empty() ) {
        JsonObject errorObject = JSON_EMPTY_OBJ;
        Errors::makeObject( Errors::Codes::APPLICATION_ERROR, "hub.ssh_tunnel.failed", errorMessage, errorObject );
        setErrorObject( joAnswer, errorObject );
    }
    else {
        m_service.getDBClient().insert( TempSupportTunnelProcessIdsDBName, JSON( "_id" << pid ) );
    }

    Log::info( utils::stringify( "Remote port forwarding ",
                                 pid < 0 ? "failed" : "succeeded",
                                 " for listen port ",
                                 tunnelPort,
                                 " on host: ",
                                 serverUser,
                                 "@",
                                 serverAddress ) );
}

// params: {}
void HubSshTunnelCommandsHandler::handleDeleteAllRequest( rpc::Message& /*rpcMessage*/ ) {
    auto& dbClient = m_service.getDBClient();
    auto tunnelProcessIds = dbClient.findAll( TempSupportTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    for( const auto& jsonPid : tunnelProcessIds ) {
        auto pid = json_get_int( jsonPid, "_id" );
        Ssh::TunnelManager::killSshTunnel( pid );
    }
    dbClient.clear( TempSupportTunnelProcessIdsDBName );
}

}  // namespace Uid
}  // namespace HubPlatform
