#include "uid/server/server_listener.h"
#include "http_bridge/ws_api_bridge.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"
#include "base_utils/utils.h"

#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_url.h>
#include <utility>

bool server_listener::g_bAcceptConnectionsFromLocalNetworkOnly = true;

namespace {
/* current implementation of lws library (3.1.0) has an issue which causes an http connection to be closed
 * since ~6 minutes after it was open, to handle this issue we override a default
 * lws_context_creation_info::timeout_secs_ah_idle value */
std::chrono::seconds lwsAHIdleTimeout = std::chrono::minutes( 30 );
}  // namespace

server_listener::server_listener( server_peer::on_message_t onMessage,
                                  const server_vhost_settings& secureHost,
                                  const server_vhost_settings& insecureHost,
                                  server_listener_base::onBeforeCreateContext_t onBeforeCreateContext )
: m_onMessage( std::move( onMessage ) )
, m_onBeforeCreateContext( std::move( onBeforeCreateContext ) )
, m_defaultHost( secureHost )
, m_insecureHost( insecureHost ) {
    setUp();
}

server_listener::server_listener( server_peer::on_message_t onMessage,
                                  const tiny::ws::security_args& sa,
                                  const std::string& strServerUrlScheme,
                                  const int nSocketListenPort,  //  = 17000
                                  server_listener_base::onBeforeCreateContext_t beforeCreateContextHandler )
: m_onMessage( std::move( onMessage ) )
, m_onBeforeCreateContext( std::move( beforeCreateContextHandler ) ) {
    m_defaultHost.sa = sa;
    m_defaultHost.uri = strServerUrlScheme;
    m_defaultHost.port = nSocketListenPort;

    m_insecureHost.port = 0;

    setUp();
}

void server_listener::setUp() {
    LOGGER_DEBUG_RAW( "server_listener::setUp" );
    tiny::ws::security_args& sa_this = ( *this );
    sa_this = m_defaultHost.sa;

    if( Db::getFlag( OfflineInsecureAccessFlagName ) ) {
        tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ = true;
        tiny::ws::basic_network_settings::default_instance().ssl_server_redirect_http_2_https_ = false;
    }
    else {
        tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ = false;
        tiny::ws::basic_network_settings::default_instance().ssl_server_redirect_http_2_https_ = true;
    }

    //
    Log::info( cc::debug( "Will start " ) +
               ( ( m_defaultHost.uri == "wss" ) ? cc::success( m_defaultHost.uri ) : cc::fatal( m_defaultHost.uri ) ) +
               cc::debug( " " ) + cc::info( "LocalNMA" ) + cc::debug( " server on port " ) +
               cc::num10( m_defaultHost.port ) );
    onPeerInstantiate_ = [&]( server_listener_base& srv, tiny::ws::server::hdl_t h ) -> tiny::ws::server::peer_ptr_t {
        tiny::ws::server::peer_ptr_t pPeer( nullptr );
        std::string strPeerDescription = "NULL-PEER";
        try {
            pPeer = new server_peer( srv, h, m_onMessage );
            strPeerDescription = pPeer->getShortPeerDescription();
            // Log::error( cc::error("Got peer and peer description"));
        }
        catch( const std::exception& e ) {
            const char* strWhat = e.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::error( cc::error( "on-open(" ) + cc::info( strPeerDescription ) +
                        cc::error( ") standard exception: " ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::error( cc::error( "on-open(" ) + cc::info( strPeerDescription ) + cc::error( ") generic exception" ) );
        }
        if( !pPeer ) {
            // we are here on error only
            try {
                srv.close( h,
                           tiny::ws::server::close_status::internal_endpoint_error,
                           "internal peer allocation error at server side(inside micro NMA listener)" );
            }
            catch( ... ) {
            }
            return pPeer;
        }
        return pPeer;
    };
    m_serverOpenComplete.exchange( false );

    onBeforeCreateContext(
        [this]( const lws_protocols*& protocols, unsigned& length, const lws_http_mount*& mount ) -> void {
            LOGGER_TRACE_RAW( "onBeforeCreateContext called from server_listener::server_listener" );
            if( m_onBeforeCreateContext != nullptr )
                m_onBeforeCreateContext( protocols, length, mount );
        } );

    m_serverThread = std::thread( [&]() {
        //        int actualPort = nSocketListenPort;
        int actualPort = m_defaultHost.port;
        lws_context_configuration lwsCtxConfig;
        lwsCtxConfig.ahIdleTimeout = lwsAHIdleTimeout;

        if( open_autofind_port( m_defaultHost.uri, actualPort, lwsCtxConfig ) ) {
            m_defaultHost.port = actualPort;
            Log::info( cc::success( "Done" ) + cc::debug( ", " ) + cc::info( "LocalNMA" ) +
                       cc::debug( " server started" ) );

            {
                Log::info( "Try to open non-secure vhost" );
                server_vhost_settings settings( m_insecureHost.sa, m_insecureHost.port, "ws" );
                if( server::appendVHost( settings ) ) {
                    m_insecureHost.port.exchange( settings.port );
                    Log::info( cc::info( "VHost created successfully on port " ) +
                               cc::note( cc::num10( settings.port ) ) );
                }
                else {
                    Log::error( cc::error( "Could not create a non-secure vhost on port " ) +
                                cc::num( settings.port ) );
                }
            }

            m_serverOpenComplete.exchange( true );

            utils::AfterReturn onReturn( [this]() { m_serverOpenComplete.exchange( false ); } );
            if( service_mode_supported() ) {
                Log::info( cc::info( "LocalNMA main loop" ) + cc::debug( " will run in poll in service mode" ) );
                service( [&]() -> bool { return ( !/*tiny::signal::g_bStop*/ m_bStopFlag ) ? true : false; } );
            }
            else {
                Log::info( cc::info( "LocalNMA main loop" ) + cc::debug( " will run in poll/reset mode" ) );
                for( ; ( !m_bStopFlag ) && ( !tiny::signal::g_bStop ); ) {
                    poll( [&]() -> bool { return ( ( !m_bStopFlag ) && ( !tiny::signal::g_bStop ) ) ? true : false; } );
                    reset();
                }  // for( ; (!bStopFlag_) && (!tiny::signal::g_bStop); )
            }
            Log::info( cc::info( "LocalNMA main loop" ) + cc::debug( " finish" ) );
        }
        else
            Log::fatal( cc::fatal( "Fail" ) + cc::error( ", " ) + cc::info( "LocalNMA" ) +
                        cc::error( " server was not started" ) );
    } );
    Log::info( cc::debug( "Waiting for " ) + cc::info( "LocalNMA" ) + cc::debug( " server open..." ) );
    size_t nAttemptIndex, nAttemptsCount = 20, nSleepStemMilliseconds = 100;
    for( nAttemptIndex = 0; ( !m_serverOpenComplete ) && nAttemptIndex < nAttemptsCount; ++nAttemptIndex )
        std::this_thread::sleep_for( std::chrono::milliseconds( nSleepStemMilliseconds ) );
    if( m_serverOpenComplete ) {
        Log::info( cc::success( "Done" ) + cc::debug( ", " ) + cc::info( "LocalNMA" ) +
                   cc::debug( " server is running on port " ) + cc::num10( m_defaultHost.port ) );
    }
    else
        Log::warn( cc::warn( "Timeout" ) + cc::warn( ", " ) + cc::info( "LocalNMA" ) + cc::warn( " server is still " ) +
                   cc::error( "not started" ) );
}

server_listener::~server_listener() {
    Log::info( cc::debug( "Will stop " ) + cc::note( "LocalNMA" ) + cc::debug( " server..." ) );
    m_bStopFlag = true;
    service_interrupt();
    try {
        if( m_serverThread.joinable() ) {
            m_serverThread.join();
        }
    }
    catch( ... ) {
    }
    Log::info( cc::success( "Success" ) + cc::debug( ", " ) + cc::note( "LocalNMA" ) +
               cc::debug( " server did stop" ) );
}

void server_listener::onOpen( tiny::ws::server::hdl_t hdl ) {
    if( g_bAcceptConnectionsFromLocalNetworkOnly ) {
        std::string strRemoteIp = getRemoteIp( hdl );
        if( ( !tiny::is_local_private_network_address( strRemoteIp ) ) &&
            ( !tiny::is_local_private_network_url( strRemoteIp ) ) && strRemoteIp != "ws://::1" &&
            strRemoteIp != "wss://::1" && strRemoteIp != "ws://unknown" &&
            strRemoteIp != "wss://unknown" ) {  // temporary fix unknown strRemoteIp
            Log::fatal( cc::fatal( "SECURITY VIOLATION" ) + cc::error( ", " ) + cc::fatal( "non-local" ) +
                        cc::error( " peer " ) + cc::u( strRemoteIp ) + cc::error( " will be dropped " ) );
            server_listener_base::close(
                hdl, tiny::ws::server::close_status::force_tcp_drop, "non-local IPs are disabled here" );
            return;
        }
    }  // if( g_bAcceptConnectionsFromLocalNetworkOnly )
    server_listener_base::onOpen( hdl );
}

void server_listener::onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    switch( eWSLMT ) {
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_debug:
        Log::debug( msg );
        return;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_info:
        Log::info( msg );
        return;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_warning:
        Log::warn( msg );
        return;
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_error:
        Log::error( msg );
        return;
    }  // switch( eWSLMT )
    Log::debug( msg );
}

size_t server_listener::sendMessageToAllLoggedInPeers(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    server_peer::stat_locked_for_each_peer( [&]( server_peer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->sendMessage( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}
size_t server_listener::postMessageToAllLoggedInPeers(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    server_peer::stat_locked_for_each_peer( [&]( server_peer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->postMessage( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}

size_t server_listener::sendMessageToAllLoggedInPeersNoLog(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    server_peer::stat_locked_for_each_peer( [&]( server_peer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->sendMessageNoLog( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}
size_t server_listener::postMessageToAllLoggedInPeersNoLog(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    server_peer::stat_locked_for_each_peer( [&]( server_peer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->postMessageNoLog( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}
int server_listener::getPort() const {
    return m_defaultHost.port;
}
int server_listener::getInsecurePort() const {
    return m_insecureHost.port;
}

bool server_listener::getServerOpenComplete() const {
    return m_serverOpenComplete;
}

HubPlatform::HttpBridge::ApiBridge& server_listener::getApiBridge() {
    return HubPlatform::HttpBridge::getApiBridge();
}
