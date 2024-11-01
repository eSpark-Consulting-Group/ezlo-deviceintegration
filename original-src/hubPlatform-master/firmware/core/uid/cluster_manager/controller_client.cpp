#include "uid/cluster_manager/controller_client.h"
#include "uid/cluster_manager/controller_connection.h"
#include "uid/nma_client.h"
#include "uid/ui_service.h"
#include "uid/ui_proto.h"

#include "common_utils/hub_info_helper.h"

namespace HubPlatform {
namespace Uid {

namespace {
const auto HubRegistrationTimeout = std::chrono::seconds( 30 );
}

ControllerClient::ControllerClient( ControllerConnection& controllerConnection )
: tiny::ws::client::client( "" )
, m_controllerConnection( controllerConnection )
, m_isHubRegistered( false )
, m_connectionProblem( false )
, m_destructed( new bool( false ) ) {}

const std::string& ControllerClient::getControllerId() const {
    return m_controllerConnection.getControllerId();
}

std::string ControllerClient::getControllerUuid() const {
    return m_controllerConnection.getControllerUuid();
}

void ControllerClient::onRestartConnectionTimer() {
    establishConnection();
}

void ControllerClient::establishConnection() {
    if( !m_connectionMutex.try_lock() ) {
        return;
    }

    if( !m_controllerConnection.enabled() || isHubRegistered() ) {
        return;
    }

    std::lock_guard< std::mutex > connectionLock( m_connectionMutex, std::adopt_lock );

    m_connectionProblem = false;

    auto serverUrl = m_controllerConnection.getNextAddress();
    Log::debug( cc::debug( "Trying to connect to controller " ) + cc::note( getControllerId() ) + cc::debug( " at " ) +
                cc::u( serverUrl ) );

    static_cast< tiny::ws::security_args& >( *this ) = m_controllerConnection.getSecurityArgs();
    if( auto env = m_controllerConnection.getUiService().getEnvironment() )
        setClientBufferSizeRx( env->getNmaClientRxBufferSize() );

    validateOrganizationName( "controller" );
    validateOrganizationalUnitName( getControllerUuid() );
    // NOTE: temporary disable check of common name, current certificates do not contain serial there
    // validateCommonName( getControllerId() );

    auto lock = m_controllerConnection.lockCertificateFiles();
    bool isConnectedToServer = open( serverUrl );
    if( isConnectedToServer ) {
        std::unique_lock< std::mutex > registrationLock( m_registrationMutex );
        m_registrationConditionVariable.wait_for( registrationLock, HubRegistrationTimeout, [this]() {
            return !m_controllerConnection.enabled() || m_connectionProblem || m_isHubRegistered;
        } );
    }

    if( !isHubRegistered() && m_controllerConnection.enabled() ) {
        close();
        enableRestartTimer( true );
    }
}

bool ControllerClient::processCmd( rpc::Message& rpcMessage ) {
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

void ControllerClient::onOpen( tiny::ws::client::hdl_t hdl ) {
    Base::onOpen( hdl );

    Log::info( cc::info( "The connection to controller " ) + cc::note( getControllerId() ) + cc::info( " is " ) +
               cc::success( "established" ) );

    sendLoginRequest();
}

void ControllerClient::sendLoginRequest() {
    JsonObject message = prepareMethod( "hub.offline.login.hub" );
    {
        std::lock_guard< std::mutex > lock( m_registrationMutex );
        m_loginRequestId = json_get_str( message, "id" );
    }
    JsonObject joParams = JSON_EMPTY_OBJ;
    json_add_str( joParams, "hub", HubInfoHelper::getControllerUUID() );
    json_add_str( joParams, "token", m_controllerConnection.getNextToken() );
    message[ "params" ] = std::move( joParams );

    sendMessage( message.dump() );
}

void ControllerClient::handleLoginResponse( rpc::Message& rpcMessage ) {
    // Should we ignore id?
    const JsonObject& joMessage = rpcMessage.getMessage();
    rpcMessage.getAnswer() = nullptr;

    {
        std::lock_guard< std::mutex > lock( m_registrationMutex );
        if( m_loginRequestId != json_get_str( joMessage, "id" ) )
            return;
    }

    auto& error = joMessage[ "error" ];
    if( error.empty() ) {
        if( m_controllerConnection.isExternalControllerLoggingIn() ) {
            if( getControllerId() >= HubInfoHelper::getSerial() ) {
                Log::info( cc::info( "Already have connection from controller " ) + cc::note( getControllerId() ) +
                           cc::info( ", prefer keeping that connection" ) );
                closeConnection();
            }
            else {
                Log::info( cc::info( "Already have connection from controller " ) + cc::note( getControllerId() ) +
                           cc::info( ", expecting other controller to disconnect" ) );
                setHubRegistered( true );
            }
        }
        else {
            setHubRegistered( true );
        }
    }
    else {
        sendLoginRequest();
    }
}

void ControllerClient::onDestruct() {
    *m_destructed = true;
    m_registrationConditionVariable.notify_one();
    std::lock_guard< std::mutex > connectionLock( m_connectionMutex );
    close();
}

void ControllerClient::onClose( tiny::ws::client::hdl_t hdl,
                                const std::string& reason,
                                int local_close_code,
                                const std::string& local_close_code_as_str ) {
    Log::warn( cc::warn( "Closed connection to controller " ) + cc::note( getControllerId() ) + cc::debug( " at " ) +
               cc::u( uri() ) + cc::warn( ", reason is \"" ) + cc::info( reason ) + cc::warn( "\"" ) +
               cc::warn( ", close code is " ) + cc::c( local_close_code ) +
               cc::warn( ", close code explanation is \"" ) + cc::info( local_close_code_as_str ) + cc::warn( "\"" ) );

    m_connectionProblem = true;
    setHubRegistered( false );

    Base::onClose( hdl, reason, local_close_code, local_close_code_as_str );
}

void ControllerClient::onFail( tiny::ws::client::hdl_t hdl ) {
    Log::error( cc::error( "Connection failure to controller " ) + cc::note( getControllerId() ) + cc::debug( " at " ) +
                cc::u( uri() ) );

    m_connectionProblem = true;
    setHubRegistered( false );

    Base::onFail( hdl );
}

void ControllerClient::onMessage( tiny::ws::client::hdl_t hdl,
                                  tiny::ws::client::opcv eOpCode,
                                  const std::string& msg ) {
    Base::onMessage( hdl, eOpCode, msg );  // for traffic_stats
    logMessage( true, msg, eOpCode );
    auto destructed = m_destructed;
    m_controllerConnection.getTaskLoop().addTask( [this, destructed, msg]() -> void {
        try {
            if( *destructed ) {
                return;
            }
            JsonObject joMessage;
            if( !parseJson( msg, joMessage ) ) {
                Log::error( cc::error( "Controller " ) + cc::note( getControllerId() ) +
                            cc::error( " signal web socket got bad JSON: " ) + cc::j( msg ) );
                return;
            }

            icc_client_message msgIcc( this );
            msgIcc.setMessage( joMessage );
            msgIcc.answerPrepare();

            // Login result
            bool isHandled = false;
            isHandled = processCmd( msgIcc );
            if( !isHandled ) {
                isHandled = getIccManager().processDirectCmd( msgIcc, getControllerUuid() );
            }
            if( !isHandled ) {
                isHandled = getUiProto().processCmd( msgIcc );
            }
            if( !isHandled ) {
                Log::error( cc::error( "Received unsupported command: " ) + cc::note( msgIcc.getMethod() ) );
            }
            msgIcc.postAnswer( true );
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Processing controller " ) + cc::note( getControllerId() ) +
                        cc::error( " message: unknown std::exception(" ) + cc::note( typeid( ex ).name() ) +
                        cc::error( "): " ) + ex.what() );
        }
        catch( ... ) {
            Log::error( cc::error( "Processing controller " ) + cc::note( getControllerId() ) +
                        cc::error( " message: caught unknown exception." ) );
        }
    } );
}

bool ControllerClient::sendMessage( const std::string& msg,
                                    tiny::ws::client::opcv eOpCode /*= tiny::ws::client::opcv::text*/ ) {
    if( Base::sendMessage( msg, eOpCode ) ) {
        logMessage( false, msg, eOpCode );
        return true;
    }
    return false;
}

bool ControllerClient::postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    auto destructed = m_destructed;
    m_controllerConnection.getTaskLoop().addTask( [this, destructed, msg, eOpCode]() -> void {
        if( *destructed ) {
            return;
        }
        try {
            sendMessage( msg, eOpCode );
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::warn( cc::error( "ControllerClient: PostMessage: caught exception:" ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::warn( cc::error( "ControllerClient: PostMessage: caught unknown exception." ) );
        }
    } );
    return true;
}

bool ControllerClient::isHubRegistered() const {
    std::lock_guard< std::mutex > lock( m_registrationMutex );
    return m_isHubRegistered;
}

void ControllerClient::setHubRegistered( bool isHubRegistered ) {
    {
        std::lock_guard< std::mutex > lock( m_registrationMutex );
        m_loginRequestId.clear();
        m_isHubRegistered = isHubRegistered;
    }
    m_registrationConditionVariable.notify_one();

    if( isHubRegistered ) {
        Log::debug( cc::debug( "Logged in to controller " ) + cc::note( getControllerId() ) );
        m_controllerConnection.getUiService().sendBroadcast( EXTERNAL_CONTROLLERS_CONNECTED,
                                                             JSON( "controllerId" << getControllerId() ),
                                                             GroupsNames::EXTERNAL_CONTROLLERS );
    }
}

void ControllerClient::onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
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

void ControllerClient::logMessage( bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    if( !log_ws_rx_tx_ )
        return;

    Log::debug( cc::rpc_log( isRead ? cc::e_rpc_log_type_t::erpcltt_ws_rx : cc::e_rpc_log_type_t::erpcltt_ws_tx,
                             "ICC",
                             uri(),
                             cc::j( msg ),
                             eOpCode == tiny::ws::client::opcv::binary ) );
}

UiProto& ControllerClient::getUiProto() {
    return m_controllerConnection.getUiService().getNmaClient()->getUiProto();
}

InterControllerCommunication::IccManager& ControllerClient::getIccManager() {
    return m_controllerConnection.getUiService().getIccManager();
}

icc_client_message::icc_client_message( ControllerClient* pClient )
: rpc::TypedClientMessage< ControllerClient >( pClient ) {}
icc_client_message::~icc_client_message() {}

}  // namespace Uid
}  // namespace HubPlatform
