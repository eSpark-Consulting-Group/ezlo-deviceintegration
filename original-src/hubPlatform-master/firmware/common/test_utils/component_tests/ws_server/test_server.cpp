#include "test_utils/component_tests/ws_server/test_server.h"

#include "log_utils/ezlo_log.h"
#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"

#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_url.h"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

#define g_mtx_for_set_all_peers tiny::get_ref_mtx()

using mutex_type_set_all_peers = tiny::multithreading::recursive_mutex_type;
using lock_type_set_all_peers = std::lock_guard< mutex_type_set_all_peers >;

namespace HubPlatform {
namespace Tests {

TestServer::TestServer( TestServerPeer::OnMessageFunction onMessage,
                        const tiny::ws::security_args& sa,
                        std::string name,
                        OnBeforeCreateContext beforeCreateContextHandler )
: m_onMessage( std::move( onMessage ) )
, m_name( std::move( name ) )
, m_isClosing( false ) {
    tiny::ws::security_args& sa_this = ( *this );
    sa_this = sa;

    this->onBeforeCreateContext( std::move( beforeCreateContextHandler ) );

    //
    onPeerInstantiate_ = [&]( TestServer_base& srv, tiny::ws::server::hdl_t h ) -> tiny::ws::server::peer_ptr_t {
        tiny::ws::server::peer_ptr_t pPeer( nullptr );
        std::string strPeerDescription = "NULL-PEER";
        try {
            pPeer = new TestServerPeer( srv, h, m_onMessage, m_name );
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
}

TestServer::~TestServer() {
    Log::info( cc::debug( "Will stop " ) + cc::note( m_name ) + cc::debug( " server..." ) );
    m_isClosing = true;
    service_interrupt();
    try {
        if( m_serverThread.joinable() ) {
            m_serverThread.join();
        }
    }
    catch( ... ) {
    }
    Log::info( cc::success( "Success" ) + cc::debug( ", " ) + cc::note( m_name ) + cc::debug( " server did stop" ) );
}

bool TestServer::startAutofindPort( const std::string& strServerUrlScheme,
                                    int& nSocketListenPort,
                                    const lws_context_configuration& lwsContextConfig ) {
    auto m = std::make_shared< std::mutex >();
    auto cv = std::make_shared< std::condition_variable >();

    Log::info(
        cc::debug( "Will start " ) +
        ( ( strServerUrlScheme == "wss" ) ? cc::success( strServerUrlScheme ) : cc::warn( strServerUrlScheme ) ) +
        cc::debug( " " ) + cc::info( m_name ) + cc::debug( " server" ) );

    m_serverThread = std::thread( [this, strServerUrlScheme, &nSocketListenPort, cv, m, lwsContextConfig]() {
        if( open_autofind_port( strServerUrlScheme, nSocketListenPort, lwsContextConfig ) ) {
            Log::info( cc::success( "Done" ) + cc::debug( ", " ) + cc::info( m_name ) +
                       cc::debug( " server started" ) );
            {
                std::unique_lock< std::mutex > lock( *m );
                m_serverStarted = true;
                cv->notify_all();
            }

            if( service_mode_supported() ) {
                Log::info( cc::info( m_name ) + cc::debug( " main loop will run in poll in service mode" ) );
                service( [&]() -> bool { return ( !/*tiny::signal::g_bStop*/ m_isClosing ) ? true : false; } );
            }
            else {
                Log::info( cc::info( m_name ) + cc::debug( " main loop will run in poll/reset mode" ) );
                for( ; ( !m_isClosing ) && ( !tiny::signal::g_bStop ); ) {
                    poll( [&]() -> bool { return ( ( !m_isClosing ) && ( !tiny::signal::g_bStop ) ) ? true : false; } );
                    reset();
                }  // for( ; (!bStopFlag_) && (!tiny::signal::g_bStop); )
            }
            Log::info( cc::info( m_name ) + cc::debug( " main loop finish" ) );
        }
        else
            Log::fatal( cc::fatal( "Fail" ) + cc::error( ", " ) + cc::info( m_name ) +
                        cc::error( " server was not started" ) );
    } );

    Log::info( cc::debug( "Waiting for " ) + cc::info( m_name ) + cc::debug( " server open..." ) );
    std::unique_lock< std::mutex > lock( *m );
    cv->wait_for( lock, std::chrono::seconds( 2 ), [this]() -> bool { return m_serverStarted; } );
    if( m_serverStarted )
        Log::info( cc::success( "Done" ) + cc::debug( ", " ) + cc::info( m_name ) +
                   cc::debug( " server is running on port " ) + cc::num10( nSocketListenPort ) );
    else
        Log::warn( cc::warn( "Timeout" ) + cc::warn( ", " ) + cc::info( m_name ) + cc::warn( " server is still " ) +
                   cc::error( "not started" ) );

    return m_serverStarted;
}

bool TestServer::isStarted() const {
    return m_serverStarted;
}

void TestServer::onOpen( tiny::ws::server::hdl_t hdl ) {
    std::string strRemoteIp = getRemoteIp( hdl );
    if( ( !tiny::is_local_private_network_address( strRemoteIp ) ) &&
        ( !tiny::is_local_private_network_url( strRemoteIp ) ) && strRemoteIp != "ws://::1" &&
        strRemoteIp != "wss://::1" ) {
        Log::fatal( cc::fatal( "SECURITY VIOLATION" ) + cc::error( ", " ) + cc::fatal( "non-local" ) +
                    cc::error( " peer " ) + cc::u( strRemoteIp ) + cc::error( " will be dropped " ) );
        TestServer_base::close(
            hdl, tiny::ws::server::close_status::force_tcp_drop, "non-local IPs are disabled here" );
        return;
    }

    TestServer_base::onOpen( hdl );
}

void TestServer::onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    switch( eWSLMT ) {
    case tiny::ws::e_ws_log_message_type_t::eWSLMT_debug:
        Log::debug( msg );
        break;
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

size_t TestServer::sendMessageToAllLoggedInPeers( const std::string& msg,
                                                  tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    stat_locked_for_each_peer( [&]( TestServerPeer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->sendMessage( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}
size_t TestServer::postMessageToAllLoggedInPeers( const std::string& msg,
                                                  tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    stat_locked_for_each_peer( [&]( TestServerPeer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->postMessage( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}

size_t TestServer::sendMessageToAllLoggedInPeersNoLog(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    stat_locked_for_each_peer( [&]( TestServerPeer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->sendMessageNoLog( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}
size_t TestServer::postMessageToAllLoggedInPeersNoLog(
    const std::string& msg,
    tiny::ws::server::opcv eOpCode /*= tiny::ws::server::opcv::text*/ ) {
    size_t cnt = 0;
    stat_locked_for_each_peer( [&]( TestServerPeer* pPeer ) -> bool {
        if( pPeer->isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            pPeer->postMessageNoLog( msg, eOpCode );
            ++cnt;
        }
        return true;  // continue enumeration
    } );
    return cnt;
}

bool TestServer::stat_peer_register( TestServerPeer* pPeer ) {
    if( !pPeer )
        return false;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itFind = m_set_all_peers.find( pPeer ), itEnd = m_set_all_peers.end();
    if( itFind != itEnd )
        return false;
    m_set_all_peers.insert( pPeer );
    pPeer->ref_retain();  // manual ref management
    return true;
}

bool TestServer::stat_peer_unregister( TestServerPeer* pPeer ) {
    if( !pPeer )
        return false;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itFind = m_set_all_peers.find( pPeer ), itEnd = m_set_all_peers.end();
    if( itFind == itEnd )
        return false;
    m_set_all_peers.erase( pPeer );
    pPeer->ref_release();  // manual ref management
    return true;
}

size_t TestServer::stat_locked_for_each_peer( TestServerPeer::fn_for_each_peer_t fn ) {
    if( !fn )
        return 0;
    size_t cnt = 0;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itWalk = m_set_all_peers.begin(), itEnd = m_set_all_peers.end();
    for( ; itWalk != itEnd; ++itWalk ) {
        TestServerPeer* pPeer = ( *itWalk );
        if( !fn( pPeer ) )
            break;
        ++cnt;
    }
    return cnt;
}

}  // namespace Tests
}  // namespace HubPlatform
