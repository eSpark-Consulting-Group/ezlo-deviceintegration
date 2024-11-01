#include "uid/nma_client.h"
#include "uid/ezlo.h"
#include "uid/ui_service.h"
#include "uid/stat_handlers.h"

#include "log_utils/ezlo_log.h"

#include "base_utils/ezlo_common.h"
#include "base_utils/ezlodb.h"
#include "uid/ui_proto.h"
#include "uid/nma_proto.h"

#include "uid/environment.h"
#include "common_utils/cpp14.h"
#include "provision_data_utils/provision_data_manager.h"

#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "database_client/dbqueryparser.h"

#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_utils.h>

#include <functional>
#include <mutex>
#include <chrono>
#include <utility>

namespace HubPlatform {
namespace Uid {

using namespace HubPlatform;
using namespace HubPlatform::HardwareManager;

bool NmaClient::g_bEnableNmaClientStatsLogging = false;
bool NmaClient::g_bSingleThreadedPeerMessages = false;
bool NmaClient::g_bSingleThreadedPeerAnswers = false;

static const size_t g_postponed_messages_db_capacity = 2000;

constexpr std::chrono::seconds REGISTER_REPEAT_INTERVAL{ 10 };

NmaClient::NmaClient( Service& uiService,
                      const tiny::ws::security_args& sa,
                      LogOutputFunction logOutFunction,
                      const std::string& serverUri )
: tiny::ws::client::client( serverUri )
, m_uiService( uiService )
, m_hardwareManager( uiService )
, m_scenesInterface( uiService )
, m_uiProto( std::make_unique< UiProto >( uiService, m_scenesInterface ) )
, m_nmaProto( std::make_unique< NmaProto >( uiService, m_scenesInterface ) )
, m_logOutFunction( std::move( logOutFunction ) )
, m_closingTimer( uiService,
                  [this]() {
                      onLogMessage( tiny::ws::e_ws_log_message_type_t::eWSLMT_info,
                                    "NMA client connection: will perform delayed close " );
                      close();
                  } )
, m_ttDisconnectionCheckPoint( std::chrono::system_clock::now().time_since_epoch() ) {
    tiny::ws::security_args& sa_this = ( *this );
    sa_this = sa;

    Log::info( cc::note( "NMA client" ) + cc::debug( " instance is " ) + cc::warn( "created" ) );
}
NmaClient::~NmaClient() {
    if( m_registerRepeatTask )
        m_registerRepeatTask->stop();
    Log::info( cc::note( "NMA client" ) + cc::debug( " instance is " ) + cc::warn( "destroyed" ) );
}

void NmaClient::onRestartConnectionTimer() {
    establishConnection( uri() );
}

void NmaClient::establishConnection( const std::string& newUrl ) {
    static std::mutex connectionMutex;

    if( !m_uiService.isRunning() ) {
        return;
    }

    if( !connectionMutex.try_lock() ) {
        return;
    }

    std::lock_guard< std::mutex > localLock( connectionMutex, std::adopt_lock );

    if( isHubRegistered() ) {
        return;
    }

    close();

    if( !m_uiService.getEnvironment()->hasProvisionInformation() ) {
        Log::info( cc::info( "Need to update controller information" ) );
        m_hardwareManager.setCloudConnection(
            HardwareManager::HubState::CloudConnection::UpdatingControllerInformation );
        m_uiService.getEnvironment()->requestProvisionInformation();
    }

    LOGGER_TRACE_RAW( cc::trace( "Try to establish connection to: " ) + cc::u( newUrl ) );

    bool isConnectedToServer = true;
    std::uint8_t attempt = 0;
    constexpr std::uint8_t maxNmaConnectionAttempts = 15;
    constexpr std::uint8_t maxAttemptForRegistration = 20;

    constexpr std::chrono::seconds connectingToNMATimeout( 3 );
    constexpr std::chrono::seconds reaskCoordinatorTimeout( 5 );
    constexpr std::chrono::seconds waitingForHubRegistrationTimeout( 3 );

    auto serverUrl = newUrl.empty() ? m_uiService.getEnvironment()->getNmaServerURL( true ) : newUrl;

    do {
        if( !isConnectedToServer || serverUrl.empty() ) {
            if( serverUrl.empty() || ( attempt >= maxNmaConnectionAttempts ) ) {
                if( !serverUrl.empty() ) {
                    Log::debug( cc::debug( "Try to ask coordinator for new NMA URL. Cannot connect to: " ) +
                                cc::u( serverUrl ) );
                }

                m_uiService.getEnvironment()->invalidateNmaServerURL();
                serverUrl = m_uiService.getEnvironment()->getNmaServerURL( true );
                attempt = 0;
            }
            else {
                std::this_thread::sleep_for( connectingToNMATimeout );
            }
        }

        if( serverUrl.empty() ) {
            isConnectedToServer = false;
            std::this_thread::sleep_for( reaskCoordinatorTimeout );
        }
        else {
            Log::info( cc::info( "[" ) + cc::note( std::to_string( attempt ) ) + cc::info( "/" ) +
                       cc::note( std::to_string( maxNmaConnectionAttempts ) ) +
                       cc::info( "] NmaClient will re-connect to: " ) + cc::u( serverUrl ) );

            // Tune ws client's rx_buffer_size
            if( auto env = m_uiService.getEnvironment() )
                setClientBufferSizeRx( env->getNmaClientRxBufferSize() );

            validateHostName( true );

            setupPingInterval();
            isConnectedToServer = open( serverUrl );
            if( isConnectedToServer ) {
                auto originalIsConnectedState = isConnected();

                for( std::uint32_t attemptForRegistration = 0;
                     isConnectedToServer && ( attemptForRegistration < maxAttemptForRegistration );
                     ++attemptForRegistration ) {
                    if( isHubRegistered() ) {
                        break;
                    }

                    Log::debug( cc::debug( "[" ) + cc::note( std::to_string( attemptForRegistration ) ) +
                                cc::debug( "/" ) + cc::note( std::to_string( maxAttemptForRegistration ) ) +
                                cc::debug( "] Waiting for hub registration" ) );
                    std::this_thread::sleep_for( waitingForHubRegistrationTimeout );

                    if( originalIsConnectedState != isConnected() ) {
                        originalIsConnectedState = isConnectedToServer = isConnected();
                    }
                    else if( !isConnected() && attemptForRegistration > 3 ) {
                        break;
                    }
                }

                if( !isHubRegistered() ) {
                    isConnectedToServer = false;
                }
            }
        }

        ++attempt;

    } while( !isHubRegistered() && m_uiService.isRunning() );

    if( m_uiService.isRunning() ) {
        enableRestartTimer( true );
    }
}

bool NmaClient::open( const std::string& uri ) {
    m_closingTimer.stop();
    Log::debug( cc::debug( "Attempting to connect to " ) + cc::u( uri ) );
    m_connecting = true;
    m_hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::Connecting );
    return Base::open( uri );
}

void NmaClient::close() {
    setHubRegistered( false );
    Base::close();
}

void NmaClient::closeConnection() {
    Base::closeConnection();
}

void NmaClient::restoreConnection() {
    Base::restoreConnection();
}

void NmaClient::delay_close( uint64_t nMilliseconds ) {
    m_closingTimer.start( std::chrono::milliseconds( nMilliseconds ), false );
}

void NmaClient::setForceReAskCloudCoordinator( bool force ) {
    m_forceReAskCloudCoordinator = force;
}

void NmaClient::setConnected( bool state ) {
    m_connecting = false;
    m_hardwareManager.setCloudConnection( state ? HardwareManager::HubState::CloudConnection::Connected :
                                                  HardwareManager::HubState::CloudConnection::NotConnected );
    Base::setConnected( state );

    if( isConnected() != state ) {
        Log::info( cc::info( "The connection to " ) + cc::u( uri() ) + cc::info( " is " ) +
                   ( state ? cc::success( "established" ) : cc::error( "broken" ) ) );
    }

    if( !state ) {
        setHubRegistered( false );
    }
}

void NmaClient::onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
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

void NmaClient::onDisconnected() {
    m_connecting = false;
    m_hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
    Base::onDisconnected();
    emitCloudDisconnectedEvent( m_activeNetworkInterface, "disconnected" );
    m_activeNetworkInterface.clear();
}

void NmaClient::onOpen( tiny::ws::client::hdl_t hdl ) {
    JsonObject activeInterface = Network::RemoteInterface( Uid::Service::getInstance() ).getActiveNetworkInterface();
    Log::info( cc::success( "Connected" ) + cc::info( " to " ) + cc::u( uri() ) + " on network interface " +
               cc::j( activeInterface ) );
    m_connecting = false;
    m_hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::Connected );
    Base::onOpen( hdl );
    m_ttDisconnectionCheckPoint = std::chrono::system_clock::now().time_since_epoch();
    m_forceReAskCloudCoordinator = false;

    m_activeNetworkInterface = json_get_str( activeInterface, "_id" );

    emitCloudConnectedEvent( m_activeNetworkInterface );
    sendMessage( m_nmaProto->getRegisterMethod() );
}

bool NmaClient::isNeedReAskCloudCoordinator() const {
    std::string strCloudCoordinatorURL = m_uiService.getEnvironment()->getCloudCoordinatorURL();
    if( strCloudCoordinatorURL.empty() )
        return false;
    if( m_forceReAskCloudCoordinator ) {
        return true;
    }
    std::chrono::system_clock::time_point ttNow = std::chrono::system_clock::now();
    auto duration_seconds = std::chrono::duration_cast< std::chrono::seconds >( ttNow.time_since_epoch() -
                                                                                m_ttDisconnectionCheckPoint.load() );
    auto seconds = duration_seconds.count();
    time_t ttCloudCoordinatorReconnectPeriodInSeconds =
        m_uiService.getEnvironment()->getCloudCoordinatorReconnectPeriod();

    return seconds >= ttCloudCoordinatorReconnectPeriodInSeconds;
}

void NmaClient::setupPingInterval() {
    interval_ping_ = m_nmaProto->getNmaPingInterval();
}

void NmaClient::onClose( tiny::ws::client::hdl_t hdl,
                         const std::string& reason,
                         int local_close_code,
                         const std::string& local_close_code_as_str ) {
    Log::warn( cc::warn( "Closed connection to " ) + cc::u( uri() ) + cc::warn( ", reason is \"" ) +
               cc::info( reason ) + cc::warn( "\"" ) + cc::warn( ", close code is " ) + cc::c( local_close_code ) +
               cc::warn( ", close code explanation is \"" ) + cc::info( local_close_code_as_str ) + cc::warn( "\"" ) );

    m_connecting = false;
    m_hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
    bool isInternetConnected = Network::RemoteInterface( Uid::Service::getInstance() ).isInternetAvailable();
    emitCloudDisconnectedEvent( m_activeNetworkInterface, "close" );
    m_activeNetworkInterface.clear();

    setConnected( false );

    bool needReAskCloudCoordinator = isNeedReAskCloudCoordinator();
    if( needReAskCloudCoordinator ) {
        m_ttDisconnectionCheckPoint = std::chrono::system_clock::now().time_since_epoch();
        enableRestartTimer( false );
    }

    Base::onClose( hdl, reason, local_close_code, local_close_code_as_str );

    if( needReAskCloudCoordinator ) {
        m_uiService.addTask( [this, isInternetConnected]() {
            resetServerUrl();
            m_uiService.getEnvironment()->invalidateNmaServerURL();
            enableRestartTimer( isInternetConnected );
        } );
    }

    enableRestartTimer( isInternetConnected );
}

void NmaClient::onFail( tiny::ws::client::hdl_t hdl ) {
    Log::error( cc::error( "Connection failure to " ) + cc::u( uri() ) );

    m_connecting = false;
    m_hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );

    setHubRegistered( false );
    bool needReAskCloudCoordinator = isNeedReAskCloudCoordinator();

    if( needReAskCloudCoordinator ) {
        m_ttDisconnectionCheckPoint = std::chrono::system_clock::now().time_since_epoch();
        enableRestartTimer( false );
    }

    Base::onFail( hdl );

    if( needReAskCloudCoordinator ) {
        resetServerUrl();
        m_uiService.getEnvironment()->invalidateNmaServerURL();
    }

    m_uiService.addTask( [this]() -> void {
        Log::info( cc::info( "Will use standard restart timer after fail" ) );
        enableRestartTimer( true );
    } );

    emitCloudDisconnectedEvent( m_activeNetworkInterface, "fail" );
    m_activeNetworkInterface.clear();
}

void NmaClient::onMessage( tiny::ws::client::hdl_t hdl, tiny::ws::client::opcv eOpCode, const std::string& msg ) {
    Base::onMessage( hdl, eOpCode, msg );  // for traffic_stats
    m_ttDisconnectionCheckPoint = std::chrono::system_clock::now().time_since_epoch();
    if( m_logOutFunction )
        m_logOutFunction( *this, true, msg, eOpCode );
    auto fnHandler = []( NmaClient& ws, std::string msg ) -> void {
        try {
            JsonObject joMessage;
            if( !parseJson( msg, joMessage ) ) {
                Log::error( cc::error( "Signal web socket got bad JSON: " ) + cc::j( msg ) );
                return;
            }

            enum class SenderType { No, Ui, Controller, Other };
            auto sender = SenderType::No;

            JsonObject joSender = JSON_EMPTY_OBJ;
            std::string stringSenderId;

            if( json_get_obj( joMessage, "sender", joSender ) ) {
                auto senderType = json_get_str( joSender, "type" );
                if( senderType == "ui" ) {
                    sender = SenderType::Ui;
                }
                else if( senderType == "controller" ) {
                    sender = SenderType::Controller;
                }
                else {
                    sender = SenderType::Other;
                }
            }
            else if( json_get( joMessage, "sender", stringSenderId ) ) {
                if( !stringSenderId.empty() ) {
                    sender = SenderType::Other;
                };
            }

            nma_client_message msgUId( &ws );
            msgUId.setMessage( joMessage );
            msgUId.answerPrepare();

            bool isHandled = false;
            switch( sender ) {
            case SenderType::No:
                isHandled = ws.m_nmaProto->processCmd( msgUId );
                break;
            case SenderType::Controller:
                isHandled = ws.m_uiService.getIccManager().processCloudCmd( msgUId );
                if( isHandled ) {
                    break;
                }
            case SenderType::Ui:
            case SenderType::Other:
                isHandled = ws.m_uiProto->processCmd( msgUId );
                break;
            }

            bool redirected = false;
            if( !isHandled ) {
                redirected = ws.m_uiService.redirectCloudMessage( msgUId );
            }

            if( !redirected ) {
                if( !isHandled ) {
                    Log::error( cc::error( "Received unsupported command: " ) + cc::note( msgUId.getMethod() ) );
                }
                msgUId.postAnswer( true );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "Processing NMA message: unknown std::exception(" ) +
                        cc::note( typeid( ex ).name() ) + cc::error( "): " ) + ex.what() );
        }
        catch( ... ) {
            Log::error( cc::error( "Processing NMA message: caught unknown exception." ) );
        }
    };
    if( g_bSingleThreadedPeerMessages ) {
        fnHandler( std::ref( *this ), msg );
    }
    else {
        m_uiService.addTask( std::bind( std::move( fnHandler ), std::ref( *this ), std::move( msg ) ) );
    }
}

bool NmaClient::sendMessage( const std::string& msg,
                             tiny::ws::client::opcv eOpCode /*= tiny::ws::client::opcv::text*/ ) {
    if( Base::sendMessage( msg, eOpCode ) ) {
        if( m_logOutFunction )
            m_logOutFunction( *this, false, msg, eOpCode );
        return true;
    }
    return false;
}

bool NmaClient::postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    if( !isHubRegistered() ) {
        postponeMessage( msg, eOpCode );
        return false;
    }

    if( g_bSingleThreadedPeerAnswers )
        return sendMessage( msg, eOpCode );

    m_uiService.addTask( [this, msg, eOpCode]() -> void {
        try {
            sendMessage( msg, eOpCode );
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::warn( cc::error( "NmaClient: PostMessage: caught exception:" ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::warn( cc::error( "NmaClient: PostMessage: caught unknown exception." ) );
        }
    } );
    return true;
}

void NmaClient::postponeMessage( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    const Json jsonMessage = parseJson( msg );
    if( !jsonMessage.empty() ) {
        using namespace std::chrono;
        auto timeStamp = std::to_string( steady_clock::now().time_since_epoch().count() );
        const bool isBroadcastMessage = json_get_str( jsonMessage, "id" ) == "ui_broadcast";
        bool isUserNotification = false;
        auto resultIt = jsonMessage.find( "result" );
        if( resultIt != jsonMessage.end() ) {
            isUserNotification = json_get_bool( *resultIt, "userNotification" );
        }
        auto insertMessage = [&]() -> void {
            Db::insert( TempPostponedMessagesDBName,
                        JSON( db::ID_FIELD_NAME << timeStamp << "message" << jsonMessage.dump() << "eOpCode" << eOpCode
                                                << "isBroadcastMessage" << isBroadcastMessage << "isUserNotification"
                                                << isUserNotification ) );
        };
        std::lock_guard< std::mutex > lock( m_postponedMutex );
        if( Db::count( TempPostponedMessagesDBName, JSON_EMPTY_OBJ ) < g_postponed_messages_db_capacity ) {
            insertMessage();
        }
        else {
            // Replace old non-broadcast message with new message
            auto oldOtherMessage = Db::findOne( TempPostponedMessagesDBName, JSON( "isBroadcastMessage" << false ) );
            if( !oldOtherMessage.empty() ) {
                Db::remove( TempPostponedMessagesDBName, oldOtherMessage, true );
                insertMessage();
            }
            // If there's no non-broadcast message then if message is broadcast or broadcast with userNotification set
            // to true then replace old broadcast message
            else if( isBroadcastMessage || isUserNotification ) {
                auto oldBroadcastMessage =
                    Db::findOne( TempPostponedMessagesDBName,
                                 JSON( "isBroadcastMessage" << true << "isUserNotification" << false ) );
                if( !oldBroadcastMessage.empty() ) {
                    Db::remove( TempPostponedMessagesDBName, oldBroadcastMessage, true );
                    insertMessage();
                }
                // If there's no old broadcast message that is not user notification and new message is user
                // notification then replace oldest message
                else if( isUserNotification ) {
                    Db::remove( TempPostponedMessagesDBName, JSON_EMPTY_OBJ, true );
                    insertMessage();
                }
            }
        }
    }
}

void NmaClient::postPostponedMessages() {
    std::unique_lock< std::mutex > lock( m_postponedMutex );
    while( Db::count( TempPostponedMessagesDBName, JSON_EMPTY_OBJ ) ) {
        auto record = Db::findOne( TempPostponedMessagesDBName, JSON_EMPTY_OBJ );
        lock.unlock();
        if( !postMessage( json_get_str( record, "message" ),
                          static_cast< tiny::ws::client::opcv >( json_get_int( record, "eOpCode" ) ) ) ) {
            break;
        }
        lock.lock();
        Db::remove( TempPostponedMessagesDBName, record, true );
    }
}

bool NmaClient::sendMessageNoLog( const std::string& msg,
                                  tiny::ws::client::opcv eOpCode /*= tiny::ws::client::opcv::text*/ ) {
    return Base::sendMessage( msg, eOpCode );
}

bool NmaClient::postMessageNoLog( const std::string& msg, tiny::ws::client::opcv eOpCode ) {
    if( !isHubRegistered() )
        return false;

    if( g_bSingleThreadedPeerAnswers )
        return sendMessageNoLog( msg, eOpCode );

    m_uiService.addTask( std::bind(
        []( NmaClient* pClient, const std::string& msg, tiny::ws::client::opcv eOpCode ) -> void {
            try {
                pClient->sendMessageNoLog( msg, eOpCode );
            }
            catch( const std::exception& ex ) {
                const char* strWhat = ex.what();
                if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                    strWhat = "unknown exception";
                std::cout << cc::error( "NmaClient: PostMessageNoLog: caught exception:" ) + cc::warn( strWhat )
                          << std::endl;
            }
            catch( ... ) {
                std::cout << cc::error( "NmaClient: PostMessageNoLog: caught unknown exception." ) << std::endl;
            }
        },
        this,
        std::move( msg ),
        eOpCode ) );
    return true;
}

UiProto& NmaClient::getUiProto() {
    return *m_uiProto;
}

NmaProto& NmaClient::getNmaProto() {
    return *m_nmaProto;
}

bool NmaClient::isHubRegistered() const {
    return m_isHubRegistered;
}

void NmaClient::setHubRegistered( bool isHubRegistered ) {
    m_isHubRegistered = isHubRegistered;

    m_uiService.onHubRegistered( isHubRegistered );

    if( isHubRegistered ) {
        const auto currentUrl = uri();
        if( currentUrl != m_uiService.getEnvironment()->getExplicitNmaServerURL() ) {
            m_uiService.getEnvironment()->setExplicitNmaServerURL( currentUrl );
        }

        postPostponedMessages();

        if( m_registerRepeatTask ) {
            m_registerRepeatTask->stop();
            m_registerRepeatTask.reset();
            m_uiService.onHubRegisterRepeatFinished();
        }
    }
}

void NmaClient::onHubRegisterRepeat() {
    Log::info( cc::info( "Register repeat requested" ) );

    if( m_registerRepeatTask )
        m_registerRepeatTask->stop();

    m_uiService.onHubRegisterRepeatStarted();

    m_registerRepeatTask = m_uiService.createPostponedTask( [this]( Services::PostponedTask& ) {
        Log::info( cc::info( "Sending new register request" ) );
        sendMessage( m_nmaProto->getRegisterMethod() );
    } );
    m_registerRepeatTask->start( std::chrono::seconds{ 0 }, REGISTER_REPEAT_INTERVAL );
}

void NmaClient::processBroadcast( const Services::Message& message ) {
    m_uiProto->processBroadcast( message );
}

bool NmaClient::isConnecting() const {
    return m_connecting;
}

void NmaClient::emitCloudConnectedEvent( const std::string& interfaceName ) {
    using namespace std::chrono;
    auto timePointSec = system_clock::to_time_t( system_clock::now() );
    auto steadyTime = duration_cast< seconds >( steady_clock::now().time_since_epoch() );
    m_uiService.sendBroadcast(
        NMA_CLIENT_CONNECTED,
        JSON( "activeInterface" << interfaceName << "now" << timePointSec << "monotonicNow" << steadyTime.count() ),
        GroupsNames::BROADCAST_GROUP );
}

void NmaClient::emitCloudDisconnectedEvent( const std::string& interfaceName, const std::string& action ) {
    using namespace std::chrono;
    auto timePointSec = system_clock::to_time_t( system_clock::now() );
    auto steadyTime = duration_cast< seconds >( steady_clock::now().time_since_epoch() );
    m_uiService.sendBroadcast( NMA_CLIENT_DISCONNECTED,
                               JSON( "activeInterface" << interfaceName << "action" << action << "now" << timePointSec
                                                       << "monotonicNow" << steadyTime.count() ),
                               GroupsNames::BROADCAST_GROUP );
}

nma_client_message::nma_client_message( NmaClient* pClient )
: rpc::TypedClientMessage< NmaClient >( pClient ) {}
nma_client_message::~nma_client_message() {}

}  // namespace Uid
}  // namespace HubPlatform
