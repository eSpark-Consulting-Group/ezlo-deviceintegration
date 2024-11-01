
#include "test_utils/component_tests/ws_server/test_server_peer.h"
#include "test_utils/component_tests/ws_server/test_server.h"

#include "test_utils/component_tests/ws_server/test_server_proto.h"
#include "test_utils/component_tests/ws_server/test_peer_message.h"

#define g_mtx_for_set_all_peers tiny::get_ref_mtx()

namespace HubPlatform {
namespace Tests {

bool TestServerPeer::g_bSingleThreadedPeerMessages = false;
bool TestServerPeer::g_bSingleThreadedPeerAnswers = false;
bool TestServerPeer::g_bEnableMicroNmaPeerStatsLogging = false;

TestServerPeer::TestServerPeer( tiny::ws::server::server& srv,
                                const tiny::ws::server::hdl_t& hdl,
                                OnMessageFunction onMessage,
                                std::string name )
: tiny::ws::server::peer( srv, hdl )
, m_onMessage( std::move( onMessage ) )
, m_name( name ) {
    get_dispatch_queue_id();  // cache
    getOrigin();
    Log::info( cc::note( m_name + " peer" ) + cc::debug( " " ) + getShortPeerDescription( true ) +
               cc::debug( " instance is " ) + cc::warn( "created" ) );
}

TestServerPeer::~TestServerPeer() {
    Log::info( cc::note( m_name + " peer" ) + cc::debug( " " ) + getShortPeerDescription( true ) +
               cc::debug( " instance is " ) + cc::warn( "destroyed" ) );
}

TestServerPeer::MutexType& TestServerPeer::peer_mtx() const {
    // return peer_mutex_;
    return g_mtx_for_set_all_peers;
}

tiny::dispatch::queue_id_t TestServerPeer::get_dispatch_queue_id() const {
    if( m_dispatchQueueId.empty() )
        m_dispatchQueueId = std::string( m_name + " peer queue " ) + unique_string_identifier();
    return m_dispatchQueueId;
}

std::string TestServerPeer::getRemoteIp() const {
    LockType lock( peer_mtx() );
    if( m_cachedRemoteIp.empty() )
        m_cachedRemoteIp = tiny::ws::server::peer::getRemoteIp();
    return m_cachedRemoteIp;
}

std::string TestServerPeer::getOrigin() const {
    LockType lock( peer_mtx() );
    if( m_cachedOrigin.empty() )
        m_cachedOrigin = tiny::ws::server::peer::getRemoteIp();
    return m_cachedOrigin;
}

const std::string& TestServerPeer::getSerial() const {  // if not empty, then logged in
    return m_hubSerial;
}

std::string TestServerPeer::getShortTypeDescrition( bool isColored /*= false*/ ) const {
    // LockType lock( peer_mtx() );
    std::string strSep( "/" ), strBraceOpen( "(" ), strBraceClose( ")" ), strType;
    if( !isLoggedIn() ) {
        strType = "<not logged>";
        if( isColored ) {
            if( !strType.empty() )
                strType = cc::warn( strType );
        }
    }
    else {
        strType = "HUB";
        if( isColored ) {
            if( !strType.empty() )
                strType = cc::info( strType );
        }
    }
    if( isColored ) {
        if( !strSep.empty() )
            strSep = cc::debug( strSep );
        if( !strBraceOpen.empty() )
            strBraceOpen = cc::debug( strBraceOpen );
        if( !strBraceClose.empty() )
            strBraceClose = cc::debug( strBraceClose );
    }
    std::stringstream ss;
    std::string hubSerial = getSerial();
    if( isColored && !hubSerial.empty() ) {
        hubSerial = cc::note( hubSerial );
    }

    ss << strType;
    if( !hubSerial.empty() )
        ss << strSep << hubSerial;

    return ss.str();
}

std::string TestServerPeer::getShortPeerDescription( bool isColored /*= false*/ ) const {
    // LockType lock( peer_mtx() );
    return tiny::ws::server::peer::getShortPeerDescription( isColored );
}

void TestServerPeer::onPeerRegister() {
    std::string strPeerDescription = getShortPeerDescription( true );
    tiny::ws::server::peer::onPeerRegister();  // traffic stats
    static_cast< TestServer& >( srv() ).stat_peer_register( make_ptr( this ) );
    Log::debug( cc::debug( "Registered peer " ) + strPeerDescription );
}

void TestServerPeer::onPeerUnregister() {  // peer will no longer receive onMessage after call to this
    std::string strPeerDescription = getShortPeerDescription( true );
    tiny::ws::server::peer::onPeerUnregister();  // traffic stats
    /* Should it be implemented in micro_nma_peer? */
    // onHandleDestruction();
    static_cast< TestServer& >( srv() ).stat_peer_unregister( make_ptr( this ) );
    Log::debug( cc::debug( "Unregistered peer " ) + strPeerDescription );
}

bool TestServerPeer::isLoggedIn() const {
    LockType lock( peer_mtx() );
    return !getSerial().empty();
}

void TestServerPeer::onHubRegistered( const std::string& serial ) {
    LockType lock( peer_mtx() );
    m_hubSerial = serial;
    Log::info( cc::debug( "Hub was registered: " ) + cc::note( m_hubSerial ) );
}

bool TestServerPeer::sendMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( eOpCode != tiny::ws::server::opcv::text )
        return false;
    if( !isConnected() )
        return false;
    if( srv().log_ws_rx_tx_ ) {
        if( eOpCode != tiny::ws::server::opcv::text ||
            ( !tiny::ws::utils::is_ping_or_pong( msg ) ) )  // do not log ping-pong
            Log::info( cc::rpc_log(
                cc::e_rpc_log_type_t::erpcltt_ws_tx,
                std::string( m_name ),
                getShortPeerDescription( true ),
                msg.size() <= 5024 ?
                    msg :
                    ( msg.substr( 0, 5024 ).append( "... (Truncated, size: " + std::to_string( msg.size() ) + ")" ) ),
                ( eOpCode == tiny::ws::server::opcv::binary ) ? true : false ) );
    }
    return tiny::ws::server::peer::sendMessage( msg, eOpCode );
}

bool TestServerPeer::postMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( g_bSingleThreadedPeerAnswers )
        return sendMessage( msg, eOpCode );
    if( !isConnected() )
        return false;
    TestServerPeer* pPeer = TestServerPeer::make_ptr( this );
    pPeer->ref_retain();  // manual ref management
    try {
        tiny::dispatch::async(
            get_dispatch_queue_id(),
            std::bind(
                []( TestServerPeer* pPeer, std::string msg, tiny::ws::server::opcv eOpCode ) -> void {
                    try {
                        pPeer->sendMessage( msg, eOpCode );
                    }
                    catch( ... ) {
                    }
                    pPeer->ref_release();  // manual ref management
                },
                TestServerPeer::make_ptr( this ),
                msg,
                eOpCode ) );
        return true;
    }
    catch( const std::exception& e ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( e.what() ) +
                    cc::error( ") while submitting message to async dispatcher (" + m_name + " peer), message " ) +
                    cc::j( msg ) );
    }
    catch( ... ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( "unknown" ) +
                    cc::error( ") while submitting message to async dispatcher (" + m_name + " peer) message " ) +
                    cc::j( msg ) );
    }
    return false;
}

bool TestServerPeer::sendMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( eOpCode != tiny::ws::server::opcv::text )
        return false;
    if( !isConnected() )
        return false;
    return tiny::ws::server::peer::sendMessage( msg, eOpCode );
}

bool TestServerPeer::postMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( g_bSingleThreadedPeerAnswers )
        return sendMessageNoLog( msg, eOpCode );
    if( !isConnected() )
        return false;
    TestServerPeer* pPeer = TestServerPeer::make_ptr( this );
    pPeer->ref_retain();  // manual ref management
    try {
        tiny::dispatch::async(
            get_dispatch_queue_id(),
            std::bind(
                []( TestServerPeer* pPeer, std::string msg, tiny::ws::server::opcv eOpCode ) -> void {
                    try {
                        pPeer->sendMessageNoLog( msg, eOpCode );
                    }
                    catch( ... ) {
                    }
                    pPeer->ref_release();  // manual ref management
                },
                TestServerPeer::make_ptr( this ),
                msg,
                eOpCode ) );
        return true;
    }
    catch( const std::exception& e ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal(
            cc::error( "exception (" ) + cc::warn( e.what() ) +
            cc::error( ") while submitting message to async dispatcher (" + m_name + " peer, NoLog), message " ) +
            cc::j( msg ) );
    }
    catch( ... ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal(
            cc::error( "exception (" ) + cc::warn( "unknown" ) +
            cc::error( ") while submitting message to async dispatcher (" + m_name + " peer, NoLog) message " ) +
            cc::j( msg ) );
    }
    return false;
}

void TestServerPeer::onMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    tiny::ws::server::peer::onMessage( msg, eOpCode );  // traffic stats
    if( eOpCode != tiny::ws::server::opcv::text ) {
        Log::error( cc::error( "Ignoring non-text message got by TestNmaPeer " ) + getShortPeerDescription( true ) );
        return;
    }
    if( srv().log_ws_rx_tx_ ) {
        if( eOpCode != tiny::ws::server::opcv::text ||
            ( !tiny::ws::utils::is_ping_or_pong( msg ) ) ) {  // do not log ping-pong
            Log::info( cc::rpc_log(
                cc::e_rpc_log_type_t::erpcltt_ws_rx,
                std::string( m_name ),
                getShortPeerDescription( true ),
                msg.size() <= 5024 ?
                    msg :
                    ( msg.substr( 0, 5024 ).append( "... Truncated, size: " + std::to_string( msg.size() ) + ")" ) ),
                ( eOpCode == tiny::ws::server::opcv::binary ) ? true : false ) );
        }
    }
    try {
        auto fnHandler = []( TestServerPeer& ws, std::string msg ) {
            try {
                JsonObject joMessage;
                if( !parseJson( msg, joMessage ) ) {
                    Log::error( cc::error( "Signal web socket got bad JSON: " ) + cc::j( msg ) );
                    return;
                }
                TestPeerMessage serverMessage( &ws );
                serverMessage.setMessage( joMessage );
                // serverMessage.answerPrepare();

                std::string strMethod;

                if( serverMessage.getMessage().count( "method" ) > 0 ) {
                    strMethod = serverMessage.getMethod();
                }

                TestServerProto serverProto;
                if( !strMethod.empty() && serverProto.has( strMethod ) ) {
                    serverProto.processCmd( serverMessage );
                }
                else {
                    ws.m_onMessage( serverMessage );
                }

                if( !serverMessage.getAnswer().empty() ) {
                    serverMessage.postAnswer( true );
                }
            }
            catch( const std::exception& exception ) {
                std::cerr << "TestServerPeer: Standard exception was caught(" << typeid( exception ).name()
                          << "): " << exception.what() << std::endl;
            }
            catch( ... ) {
                std::cerr << "TestServerPeer: Unknown exception was caught" << std::endl;
            }
            ws.ref_release();  // manual ref management
        };
        ref_retain();  // manual ref management
        if( TestServerPeer::g_bSingleThreadedPeerMessages )
            fnHandler( std::ref( *this ), msg );
        else
            tiny::dispatch::async( get_dispatch_queue_id(), std::bind( fnHandler, std::ref( *this ), msg ) );
    }
    catch( const std::exception& e ) {
        ref_release();  // manual ref management
        std::string s1 = cc::error( "exception (" ) + cc::warn( e.what() ) +
                         cc::error( ") while submitting message to async dispatcher, message " ) + cc::j( msg );
        std::string s2 = std::string( "exception (" ) + std::string( e.what() ) +
                         std::string( ") while submitting message to async dispatcher, message " ) + msg;
        std::cerr << s1 << std::endl;
        // stat_handle_async_mechanism_error( *this, msg, s1, s2 );
    }
    catch( ... ) {
        ref_release();  // manual ref management
        std::string s1 = cc::error( "exception (" ) + cc::warn( "unknown" ) +
                         cc::error( ") while submitting message to async dispatcher, message " ) + cc::j( msg );
        std::string s2 = std::string( "exception (" ) + std::string( "unknown" ) +
                         std::string( ") while submitting message to async dispatcher, message " ) + msg;
        std::cerr << s1 << std::endl;
        // stat_handle_async_mechanism_error( *this, msg, s1, s2 );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
