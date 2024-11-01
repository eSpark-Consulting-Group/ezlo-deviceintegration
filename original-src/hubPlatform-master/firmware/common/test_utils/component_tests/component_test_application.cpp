
#include "test_utils/component_tests/component_test_application.h"

#include "test_utils/test_environment.h"
#include "test_utils/component_tests/ws_server/test_server.h"
#include "test_utils/component_tests/ws_server/test_server_peer.h"
#include "test_utils/component_tests/ws_server/test_server_proto.h"

#include "system_utils/filesystem.h"

#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"

#include "common_utils/cpp14.h"
#include "jsonrpc/message.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"

#include <cassert>
#include <limits>

namespace HubPlatform {
namespace Tests {

ComponentTestApplication* ComponentTestApplication::m_instance = nullptr;

ComponentTestApplication::ComponentTestApplication( const std::string& etalonRootFolderPath )
: TestApplication( etalonRootFolderPath ) {
    m_instance = this;

    const char* const optFirmwareEtcDir = "/system/common/opt/firmware/etc";
    filesystem::copyFolder( Tests::getSourceFolder() + optFirmwareEtcDir, getCurrentTestPath() + "/etc" );

    filesystem::copyFolder( Tests::getBuildGeneratedFolder() + optFirmwareEtcDir, getCurrentTestPath() + "/etc" );

    resetBroadcastsProcessFunction();
    resetInternalBroadcastsProcessFunction();
    resetRequestsProcessFunction();
}

ComponentTestApplication::~ComponentTestApplication() {
    m_instance = nullptr;
}

ComponentTestApplication& ComponentTestApplication::get() {
    if( m_instance != nullptr ) {
        return *m_instance;
    }

    throw ComponentTestApplicationError( "NmaTestsApplication instance is not initialized!" );
}

ComponentTestApplication::HubConnectionPtr ComponentTestApplication::getHubPeer() {
    HubConnectionPtr peer;

    m_hubPeer.get( peer );
    if( peer != nullptr && !peer->isLoggedIn() ) {
        peer = nullptr;
    }

    return peer;
}

void ComponentTestApplication::sendtoHub( JsonObject request, ConnectionType connectionType ) {
    HubConnectionPtr peer;

    for( std::uint8_t attempt = 0u; attempt < 10u; ++attempt ) {
        peer = getHubPeer();
        if( peer == nullptr ) {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
        else {
            break;
        }
    }

    if( peer == nullptr ) {
        throw ComponentTestApplicationError( std::string( "Hub didn't register. Cannot send message: " ) +
                                             cc::j( request ) );
    }

    switch( connectionType ) {
    case ConnectionType::UI:
        request[ "sender" ][ "conn_id" ] = "1";
        request[ "sender" ][ "type" ] = "ui";
        break;

    case ConnectionType::NMA:
        break;
    };

    const auto sendResult = peer->sendMessage( request.dump(), tiny::ws::server::opcv::text );
    if( !sendResult ) {
        throw ComponentTestApplicationError( std::string( "Cannot send message to hub: " ) + cc::j( request ) );
    }
}

void ComponentTestApplication::sendResponseToHub( const JsonObject& request, Json result ) {
    Json response;
    response[ "id" ] = request[ "id" ];
    response[ "method" ] = request[ "method" ];
    if( request.count( "api" ) != 0 ) {
        response[ "api" ] = request[ "api" ];
    }
    response[ "result" ] = std::move( result );
    sendtoHub( response, ConnectionType::NMA );
}

void ComponentTestApplication::sendtoHub( const std::string& request, ConnectionType connectionType ) {
    JsonObject jsonMessage = parseJson( request );
    return sendtoHub( std::move( jsonMessage ), connectionType );
}

JsonObject ComponentTestApplication::sendtoHubAndGetAnswer( JsonObject request, ConnectionType connectionType ) {
    std::string messageId = assignMessageId( request );

    sendtoHub( request, connectionType );

    return fetchAnswer( request, messageId );
}

std::string ComponentTestApplication::assignMessageId( JsonObject& request ) {
    std::string messageId;
    if( !json_get( request, "id", messageId ) ) {
        messageId = db::DbId::genOID();
        request[ "id" ] = messageId;
    }
    return messageId;
}

JsonObject ComponentTestApplication::fetchAnswer( const JsonObject& request, const std::string& messageId ) {
    constexpr const unsigned timeoutSeconds = 30;
    for( uint8_t attempt = 0; attempt < 30; ++attempt ) {
        std::unique_lock< std::mutex > lock( m_answersMutex );
        const auto isLocked = m_newAnswersCondition.wait_for(
            lock, std::chrono::seconds( timeoutSeconds ), [this] { return !m_answers.empty() && isRunning(); } );

        if( !isLocked ) {
            break;
        }

        auto itFind = m_answers.find( messageId );
        if( itFind != m_answers.end() ) {
            auto answerJson = std::move( itFind->second.front() );
            itFind->second.pop_front();
            if( itFind->second.empty() ) {
                m_answers.erase( messageId );
            }
            return std::move( answerJson );
        }
        if( !m_answers.empty() ) {
            std::string answers;
            for( auto& answer : m_answers ) {
                for( auto& message : answer.second ) {
                    answers += cc::debug( " - id: " ) + cc::note( answer.first ) + cc::debug( ", message: " ) +
                               cc::j( message ) + "\n";
                }
            }
            m_answers.clear();
            throw ComponentTestApplicationError( std::string( "While waiting response for: " ) + cc::j( request ) +
                                                 ", got unclaimed answers:\n" + answers );
        }
    }

    throw ComponentTestApplicationError( std::string( "Answer have not been received. Request was: " ) +
                                         cc::j( request ) );
}

JsonObject ComponentTestApplication::sendtoHubAndGetAnswer( const std::string& request,
                                                            ConnectionType connectionType ) {
    JsonObject jsonMessage( JsonObject::parse( request ) );
    return sendtoHubAndGetAnswer( std::move( jsonMessage ), connectionType );
}

void ComponentTestApplication::setBroadcastsProcessFunction( BroadcastsProcessFunction broadcastsFunction ) {
    std::lock_guard< std::mutex > lock( m_broadcastsMutex );
    m_processBroadcastsFunc = std::move( broadcastsFunction );
}

void ComponentTestApplication::resetBroadcastsProcessFunction() {
    setBroadcastsProcessFunction( []( rpc::Message& ) {} );
}

void ComponentTestApplication::setInternalBroadcastsProcessFunction(
    InternalMessageProcessFunction broadcastsFunction ) {
    std::lock_guard< std::mutex > lock( m_internalBroadcastsMutex );
    m_processInternalBroadcastsFunc = std::move( broadcastsFunction );
}

void ComponentTestApplication::resetInternalBroadcastsProcessFunction() {
    setInternalBroadcastsProcessFunction( []( const Services::Message& ) {} );
}

void ComponentTestApplication::setRequestsProcessFunction( RequestsProcessFunction requestsFunction ) {
    std::lock_guard< std::mutex > lock( m_requestsMutex );
    m_processRequestsFunc = std::move( requestsFunction );
}

void ComponentTestApplication::resetRequestsProcessFunction() {
    setRequestsProcessFunction( []( rpc::Message& ) {} );
}

void ComponentTestApplication::setInternalMethodCallProcessFunction(
    ComponentTestApplication::InternalMethodCallProcessFunction messageFunction ) {
    std::lock_guard< std::mutex > lock( m_internalMethodCallMutex );
    m_processInternalMethodCallFunc = std::move( messageFunction );
}

void ComponentTestApplication::resetInternalMethodCallProcessFunction() {
    setInternalMethodCallProcessFunction( []( const JsonObject&, MessageSubClasses, const std::string& ) {} );
}

void ComponentTestApplication::setCloudMethod( const std::string& methodName,
                                               ComponentTestApplication::CloudProcessFunction handler ) {
    std::lock_guard< std::mutex > lock( m_cloudMutex );
    m_cloudMethods[ methodName ] = handler;
}

void ComponentTestApplication::resetCloudMethod( const std::string& methodName ) {
    std::lock_guard< std::mutex > lock( m_cloudMutex );
    m_cloudMethods.erase( methodName );
}

void ComponentTestApplication::setUpChildApplications() {
    auto& dbClient = getDBClient();
    if( !dbClient.has( TempNetworkDynamicInfoDBName, JSON( db::ID_FIELD_NAME << "hub" ) ) )
        dbClient.insert( TempNetworkDynamicInfoDBName,
                         JSON( db::ID_FIELD_NAME << "hub"
                                                 << "internetAvailable" << true ) );

    tiny::ws::security_args securityArgs;
    for( std::size_t idx = 1; idx <= 3; ++idx ) {
        Log::info( cc::debug( "Set up " ) + cc::info( "TestNMA" ) + cc::debug( " server, attempt " ) +
                   cc::num10( idx ) );

        auto onMessage = [this]( rpc::Message& peerMessage ) { handleMessage( peerMessage ); };
        auto onBeforeCreateContext =
            []( const lws_protocols*& protocols, unsigned& length, const lws_http_mount*& mount ) {
                Log::trace( "onBeforeCreateContext: adding ApiBridge" );
                using ApiBridge = HubPlatform::HttpBridge::ApiBridge;

                mount = &ApiBridge::getHttpMount();
                protocols = ApiBridge::getProtocols( length );
            };

        m_nmaServer = std::make_unique< TestServer >( onMessage, securityArgs, "TestNMA", onBeforeCreateContext );
        injectApiBridge();

        if( m_nmaServer->startAutofindPort( "ws", m_nmaPort ) ) {
            assert( m_nmaPort > 0 && m_nmaPort < std::numeric_limits< std::uint16_t >::max() );
            break;
        }

        m_nmaServer.reset();
    }

    if( !m_nmaServer || !m_nmaServer->isStarted() ) {
        throw ComponentTestApplicationError( "Cannot run local WebSocket server!" );
    }

    m_nmaServer->onPeerRegister_ = [this]( tiny::ws::server::peer_ptr_t peer ) {
        HubConnectionPtr newHubPeer;
        newHubPeer = dynamic_cast< TestServerPeer* >( peer );

        if( newHubPeer == nullptr ) {
            throw ComponentTestApplicationError( std::string( "Registered unknown peer type: " ) +
                                                 typeid( *peer ).name() );
        }

        HubConnectionPtr oldHubPeer;
        m_hubPeer.get( oldHubPeer );

        if( oldHubPeer != nullptr ) {
            throw ComponentTestApplicationError( "Double registration of hubs on one server" );
        }

        m_hubPeer.set( newHubPeer );
    };

    m_nmaServer->onPeerUnregister_ = [this]( tiny::ws::server::peer_ptr_t peer ) {
        HubConnectionPtr hubPeer;
        hubPeer = dynamic_cast< TestServerPeer* >( peer );

        if( hubPeer == nullptr ) {
            throw ComponentTestApplicationError( std::string( "Tries to unregister unknown peer type: " ) +
                                                 typeid( *peer ).name() );
        }

        HubConnectionPtr oldHubPeer;
        m_hubPeer.get( oldHubPeer );

        if( oldHubPeer != hubPeer ) {
            throw ComponentTestApplicationError( "Tries to unregister not equal hub peer!" );
        }

        m_hubPeer.set( {} );
    };

    beforeApplicationsStartup();
    setUpAdditionalApplications();

    std::vector< std::string > parameters = { std::string( "--nma-server=ws://127.0.0.1:" ) +
                                              std::to_string( m_nmaPort ) };
    if( shouldUseRandomLocalPort() ) {
        parameters.emplace_back( "--random-port" );
    }
    executeCoreService( "ha-uid", "ui_service", parameters );

    HubConnectionPtr hubPeer;
    for( std::uint8_t attempt = 0u; attempt < 100u; ++attempt ) {
        hubPeer = getHubPeer();
        if( hubPeer == nullptr ) {
            std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
        }
        else {
            break;
        }
    }

    if( hubPeer == nullptr ) {
        stop();
        throw ComponentTestApplicationError( "Hub haven't been registered!" );
    }

    afterApplicationsStartup();
}

bool ComponentTestApplication::shouldUseRandomLocalPort() {
    return false;
}

void ComponentTestApplication::beforeApplicationsStartup() {
    setCloudMethod( "cloud.storage_controller_token", [this]( rpc::Message& message ) {
        sendResponseToHub( message.getMessage(), parseJson( R"({
            "endpoints": {
                "download": [
                    "https://download-cloud-qa.ezlo.com"
                ],
                "redirect_download": [
                    "https://download-redirect-cloud-qa.ezlo.com"
                ],
                "redirect_upload": [
                    "https://upload-redirect-cloud-qa.ezlo.com"
                ],
                "redirect_upload_finished": [
                    "https://upload-redirect-finished-cloud-qa.ezlo.com"
                ],
                "upload": [
                    "https://upload-cloud-qa.ezlo.com"
                ],
                "cloud_api": [
                    "https://cloud-api-cloud-qa.ezlo.com"
                ]
            },
            "token": {
                "expires_ts": 2147483647,
                "string": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
            },
            "xapp": "96ef6687-2033-48ff-9a4b-d00f64292587"
        } )" ) );
    } );
}

void ComponentTestApplication::afterApplicationsStartup() {}

bool ComponentTestApplication::runCloudMethod( rpc::Message& message ) {
    std::unique_lock< std::mutex > lock( m_cloudMutex );
    auto it = m_cloudMethods.find( json_get_str( message.getMessage(), "method" ) );
    if( it == m_cloudMethods.end() ) {
        return false;
    }
    auto handler = it->second;
    lock.unlock();
    return TestServerProto::processCmd(
        message, [&handler]( const std::string& /*method*/, rpc::Message& rpcMessage ) { handler( rpcMessage ); } );
}

void ComponentTestApplication::tearDown() {
    TestApplication::tearDown();

    m_nmaServer.reset();

    {
        std::lock_guard< std::mutex > lock( m_answersMutex );
        m_answers.clear();
    }

    resetBroadcastsProcessFunction();
    resetInternalBroadcastsProcessFunction();
    resetRequestsProcessFunction();
}
void ComponentTestApplication::handleInternalBroadcast( const Services::Message& message ) {
    std::lock_guard< std::mutex > lock( m_internalBroadcastsMutex );
    m_processInternalBroadcastsFunc( message );
}

void ComponentTestApplication::handleInternalMethodCall( const JsonObject& json,
                                                         MessageSubClasses requestType,
                                                         const std::string& sender ) {
    std::lock_guard< std::mutex > lock( m_internalMethodCallMutex );
    m_processInternalMethodCallFunc( json, requestType, sender );
}

void ComponentTestApplication::handleMessage( rpc::Message& peerMessage ) {
    auto jsonMessage = peerMessage.getMessage();

    std::string messageId;
    if( !json_get( jsonMessage, "id", messageId ) ) {
        throw ComponentTestApplicationError( std::string( "Unknown message format: " ) + cc::j( jsonMessage ) );
    }

    if( messageId == "ui_broadcast" ) {
        std::lock_guard< std::mutex > lock( m_broadcastsMutex );
        m_processBroadcastsFunc( peerMessage );
        HttpBridge::getApiBridge().sendBroadcast( jsonMessage.dump() );
    }
    else if( runCloudMethod( peerMessage ) ) {
    }
    else {
        if( jsonMessage.count( "result" ) != 0 ) {
            {
                std::lock_guard< std::mutex > lock( m_answersMutex );
                m_answers[ messageId ].push_back( std::move( jsonMessage ) );
            }
            m_newAnswersCondition.notify_all();
        }
        else {
            std::lock_guard< std::mutex > lock( m_requestsMutex );
            m_processRequestsFunc( peerMessage );
        }
    }
}

int ComponentTestApplication::getNmaPort() const {
    return m_nmaPort;
}

void ComponentTestApplication::injectApiBridge() {
    using namespace HubPlatform::HttpBridge;
    if( !m_apiBridgeHandlers.get() ) {
        Log::info( cc::info( "injectApiBridge: handlers not initialized" ) );
    }
    else {
        auto& apiBridge = getApiBridge();
        apiBridge.resetHandlers( std::move( m_apiBridgeHandlers ) );
    }
}

void ComponentTestApplication::resetApiBridgeHandlers( HttpBridge::ApiBridgeHandlersPtr handlers ) {
    m_apiBridgeHandlers = std::move( handlers );

    if( m_apiBridgeHandlers )
        injectApiBridge();
}

ComponentTestApplication::RequiredGateways ComponentTestApplication::getRequiredGateways() const {
    return {};
}

}  // namespace Tests
}  // namespace HubPlatform
