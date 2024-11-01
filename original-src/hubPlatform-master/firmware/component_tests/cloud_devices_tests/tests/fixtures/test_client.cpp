#include "cloud_devices_tests/tests/fixtures/test_client.h"

namespace HubPlatform {
namespace Tests {

namespace {
const auto HubRegistrationTimeout = std::chrono::seconds( 5 );
}

TestClient::TestClient( Services::TaskLoop& taskLoop, OnMessageFunction onMessage, const tiny::ws::security_args& sa )
: tiny::ws::client::client( "" )
, m_taskLoop( taskLoop )
, m_onMessage( std::move( onMessage ) )
, m_isHubRegistered( false ) {
    static_cast< tiny::ws::security_args& >( *this ) = sa;
}

TestClient::~TestClient() {}

const std::string& TestClient::getControllerUuid() const {
    return m_controllerUuid;
}

const std::string& TestClient::getToken() const {
    return m_token;
}

void TestClient::onRestartConnectionTimer() {
    // Do nothing
}

bool TestClient::establishConnection( const std::string& serverUrl,
                                      const std::string& controllerUuid,
                                      const std::string& token ) {
    m_controllerUuid = controllerUuid;
    m_token = token;

    close();
    m_connectionProblem = false;

    Log::debug( cc::debug( "TestClient: Trying to connect to controller at " ) + cc::u( serverUrl ) );

    bool isConnectedToServer = open( serverUrl );
    if( isConnectedToServer ) {
        std::unique_lock< std::mutex > registrationLock( m_registrationMutex );
        m_registrationConditionVariable.wait_for(
            registrationLock, HubRegistrationTimeout, [this]() { return m_connectionProblem || isHubRegistered(); } );
    }

    return isHubRegistered();
}

bool TestClient::processCmd( rpc::Message& rpcMessage ) {
    if( rpcMessage.getId() == "ui_broadcast" ) {
        return false;
    }

    if( rpcMessage.getMessage().count( "params" ) == 0 ) {
        if( rpcMessage.getMethod() == "hub.offline.login.hub" ) {
            handleLoginResponse( rpcMessage );
            return true;
        }
    }

    return false;
}

void TestClient::onOpen( tiny::ws::client::hdl_t hdl ) {
    Base::onOpen( hdl );

    Log::info( cc::info( "TestClient: The connection to controller is " ) + cc::success( "established" ) );

    sendLoginRequest();
}

void TestClient::sendLoginRequest() {
    JsonObject message = prepareMethod( "hub.offline.login.hub" );
    JsonObject joParams = JSON_EMPTY_OBJ;
    json_add_str( joParams, "hub", getControllerUuid() );
    json_add_str( joParams, "token", getToken() );
    message[ "params" ] = std::move( joParams );

    sendMessage( message.dump() );
}

bool TestClient::handleLoginResponse( rpc::Message& rpcMessage ) {
    // Should we ignore id?
    const JsonObject& joMessage = rpcMessage.getMessage();
    rpcMessage.getAnswer() = nullptr;

    auto& error = joMessage[ "error" ];
    if( error.empty() ) {
        setHubRegistered( true );
    }
    else {
        throw std::runtime_error( "TestClient: Login failed" );
    }
    return true;
}

void TestClient::onClose( tiny::ws::client::hdl_t hdl,
                          const std::string& reason,
                          int local_close_code,
                          const std::string& local_close_code_as_str ) {
    Log::warn( cc::warn( "TestClient: Closed connection to controller at " ) + cc::u( uri() ) +
               cc::warn( ", reason is \"" ) + cc::info( reason ) + cc::warn( "\"" ) + cc::warn( ", close code is " ) +
               cc::c( local_close_code ) + cc::warn( ", close code explanation is \"" ) +
               cc::info( local_close_code_as_str ) + cc::warn( "\"" ) );

    m_connectionProblem = true;
    setHubRegistered( false );

    Base::onClose( hdl, reason, local_close_code, local_close_code_as_str );
}

void TestClient::onFail( tiny::ws::client::hdl_t hdl ) {
    Log::error( cc::error( "TestClient: Connection failure to controller at " ) + cc::u( uri() ) );

    m_connectionProblem = true;
    setHubRegistered( false );

    Base::onFail( hdl );
}

void TestClient::onMessage( tiny::ws::client::hdl_t hdl, tiny::ws::client::opcv eOpCode, const std::string& msg ) {
    Base::onMessage( hdl, eOpCode, msg );  // for traffic_stats
    logMessage( true, msg, eOpCode );
    auto fnHandler = []( TestClient& ws, std::string msg ) -> void {
        try {
            JsonObject joMessage;
            if( !parseJson( msg, joMessage ) ) {
                Log::error( cc::error( "TestClient: Controller signal web socket got bad JSON: " ) + cc::j( msg ) );
                return;
            }

            test_client_message msgTcc( &ws );
            msgTcc.setMessage( joMessage );
            // msgTcc.answerPrepare();

            // Login result
            bool isHandled = false;
            isHandled = ws.processCmd( msgTcc );
            if( !isHandled ) {
                ws.m_onMessage( msgTcc );
            }
            if( !msgTcc.getAnswer().empty() ) {
                msgTcc.postAnswer( true );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "TestClient: Processing controller message: unknown std::exception(" ) +
                        cc::note( typeid( ex ).name() ) + cc::error( "): " ) + ex.what() );
        }
        catch( ... ) {
            Log::error( cc::error( "TestClient: Processing controller message: caught unknown exception." ) );
        }
    };

    m_taskLoop.addTask( std::bind( std::move( fnHandler ), std::ref( *this ), std::move( msg ) ) );
}

bool TestClient::sendMessage( const std::string& msg,
                              tiny::ws::client::opcv eOpCode /*= tiny::ws::client::opcv::text*/ ) {
    if( Base::sendMessage( msg, eOpCode ) ) {
        logMessage( false, msg, eOpCode );
        return true;
    }
    return false;
}

bool TestClient::postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    m_taskLoop.addTask( [this, msg, eOpCode]() -> void {
        try {
            sendMessage( msg, eOpCode );
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::warn( cc::error( "TestClient: PostMessage: caught exception:" ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::warn( cc::error( "TestClient: PostMessage: caught unknown exception." ) );
        }
    } );
    return true;
}

bool TestClient::isHubRegistered() const {
    return m_isHubRegistered;
}

void TestClient::setHubRegistered( bool isHubRegistered ) {
    m_isHubRegistered = isHubRegistered;
    m_registrationConditionVariable.notify_one();
}

void TestClient::onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    switch( eWSLMT ) {
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_debug:
        Log::debug( cc::debug( msg ) );
        break;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_info:
        Log::info( cc::info( msg ) );
        return;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_warning:
        Log::warn( cc::warn( msg ) );
        return;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_error:
        Log::error( cc::error( msg ) );
        return;
    }  // switch( eWSLMT )

    Log::trace( msg );
}

void TestClient::logMessage( bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    if( !log_ws_rx_tx_ )
        return;

    Log::info( cc::rpc_log( isRead ? cc::e_rpc_log_type_t::erpcltt_ws_rx : cc::e_rpc_log_type_t::erpcltt_ws_tx,
                            "Test controller client",
                            uri(),
                            cc::j( msg ),
                            eOpCode == tiny::ws::client::opcv::binary ) );
}

test_client_message::test_client_message( TestClient* pClient )
: rpc::TypedClientMessage< TestClient >( pClient ) {}
test_client_message::~test_client_message() {}

}  // namespace Tests
}  // namespace HubPlatform
