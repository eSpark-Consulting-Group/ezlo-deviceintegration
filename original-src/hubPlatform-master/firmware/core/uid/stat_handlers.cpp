#include "uid/stat_handlers.h"
#include "uid/nma_client.h"
#include "uid/ui_service.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "common_utils/curl_wrapper.h"
#include <tiny/tiny_command_line_parser.h>
#include <sstream>

void stat_classified_msg( const std::string& /*strClassName*/, const std::string& s ) {
    Log::debug( s );
}

void stat_startup_msg( const std::string& s ) {
    stat_classified_msg( cc::note( "STARTUP" ), s );
}

void stat_shutdown_msg( const std::string& s ) {
    stat_classified_msg( cc::note( "SHUTDOWN" ), s );
}

void stat_info_msg( const std::string& s ) {
    stat_classified_msg( cc::info( "NORMAL" ), s );
}

void stat_error_msg( const std::string& s ) {
    stat_classified_msg( cc::error( "ERROR" ), s );
}

void stat_setup_emotions_on_stop() {
    if( !g_bIsStopEzloHubEmotionsOnShutdown ) {
        return;
    }
    using namespace HubPlatform;
    using namespace HubPlatform::HardwareManager;
    using namespace HubPlatform::Uid;
    HardwareManager::RemoteInterface hardware( Service::getInstance() );
    hardware.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
}

void stat_handle_shutdown( int* p_nSignalNo ) {
    static bool g_bShutdownHandled = false;
    if( g_bShutdownHandled )
        return;
    g_bShutdownHandled = true;
    if( g_fn_on_shutdown_interrupt_nma_server_ )
        g_fn_on_shutdown_interrupt_nma_server_();
    if( cc::_on_ ) {
        std::cout.flush();
        std::cout << cc::reset();
        std::cout.flush();
    }
    std::cout.flush();
    if( p_nSignalNo != nullptr ) {
        stat_shutdown_msg(
            cc::fatal( tiny::signal::signal2str( *p_nSignalNo, "Invoking shutdown actions on signal ", "", false ) ) );
        std::cout.flush();
    }
    else {
        // stat_shutdown_msg( cc::info("Invoking shutdown actions on normal exit." ) );
    }
    std::cout.flush();
    if( cc::_on_ ) {
        std::cout.flush();
        std::cout << cc::reset();
        std::cout.flush();
    }
#if( defined __EZLO_SUPPORTS_CLOUD__ )
    Cameras::Cloud::controller& instCloudController = Cameras::Cloud::getDefaultControllerInstance();
    instCloudController.stop();
#endif  // (defined __EZLO_SUPPORTS_CLOUD__)
    stat_setup_emotions_on_stop();

    try {
        if( auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient() ) {
            nmaClient->close();
            nmaClient.reset();
        }
        auto& serverListener = HubPlatform::Uid::Service::getInstance().getServerListener();
        if( serverListener ) {
            serverListener->service_interrupt();
            serverListener.reset();
        }
    }
    catch( const std::exception& exept ) {
        stat_error_msg( exept.what() );
    }
    catch( ... ) {
        stat_error_msg( "Something very bad happened in nmaclient!" );
    }

    // FTP
    ::curl_global_cleanup();

    // OpenSSL
    FIPS_mode_set( 0 );
    // ENGINE_cleanup();
    // CONF_modules_unload(1);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    // ERR_remove_state();
    ERR_free_strings();
}

void stat_handle_at_exit() {
    stat_handle_shutdown();
}

void stat_handle_signal( int nSignalNo ) {
    if( nSignalNo == SIGPIPE ) {
        return;
    }

    using UiService = HubPlatform::Uid::Service;

    const auto stopWasRaisedBefore = !UiService::getInstance().isRunning();
    tiny::signal::g_bStop = true;
    std::string strMessagePrefix = stopWasRaisedBefore ? cc::error( "\nStop flag was already raised on. " ) +
                                                             cc::fatal( "WILL FORCE TERMINATE." ) +
                                                             cc::error( " Caught (second) signal for closing. " ) :
                                                         cc::error( "\nCaught (first) signal for closing. " );
    stat_error_msg( strMessagePrefix + cc::error( tiny::signal::signal2str( nSignalNo ) ) );
    if( stopWasRaisedBefore ) {
        _exit( 13 );
    }
    else {
        UiService::getInstance().stop( UiService::StoppingMode::OnTerminationSignalReceived );
    }
    stat_handle_shutdown( &nSignalNo );
}

void stat_handle_async_mechanism_error( tiny::ws::generic_sender& ws,
                                        const std::string& msg,
                                        const std::string& s1,
                                        const std::string& s2 ) {
    try {
        Log::error( s1 );
        using namespace HubPlatform;
        HardwareManager::RemoteInterface hardwareManager( Uid::Service::getInstance() );
        hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
        JsonObject message, answer;
        parseJson( msg, message );
        answerPrepare( message, answer );
        setError( answer, Errors::RPC_CALL_QUEUE_OVERFLOW );
        if( answer.is_object() )
            ws.sendMessage( answer.dump() );
    }
    catch( ... ) {
    }
}
