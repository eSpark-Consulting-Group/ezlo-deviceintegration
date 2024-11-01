#include "uid/server/server_peer.h"
#include "uid/server/server_proto.h"
#include "uid/stat_handlers.h"
#include "uid/cluster_manager/cluster_manager.h"
#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"

#define g_mtx_for_set_all_peers tiny::get_ref_mtx()

typedef tiny::multithreading::recursive_mutex_type mutex_type_set_all_peers;
typedef std::lock_guard< mutex_type_set_all_peers > lock_type_set_all_peers;

bool server_peer::g_bSingleThreadedPeerMessages = false;
bool server_peer::g_bSingleThreadedPeerAnswers = false;
bool server_peer::g_bEnableMicroNmaPeerStatsLogging = false;

typedef std::set< server_peer* > set_peers_t;
static set_peers_t g_set_all_peers;

server_peer::server_peer( tiny::ws::server::server& srv, const tiny::ws::server::hdl_t& hdl, on_message_t onMessage )
: server_peer_base( srv, hdl )
, on_message( onMessage ) {
    get_dispatch_queue_id();  // cache
    getOrigin();
    bool offlineAnonymousAccess = Db::getFlag( OfflineAnonymousAccessFlagName );
    if( !offlineAnonymousAccess ) {
        // NOTE: set timer to disconnect peer if it won't log in within 30 seconds
        lws_set_timer_usecs( hdl_, 30 * LWS_USEC_PER_SEC );
    }
    Log::info( cc::note( "LocalNMA peer" ) + cc::debug( " " ) + getShortPeerDescription( true ) +
               cc::debug( " instance is " ) + cc::warn( "created" ) +
               cc::note( offlineAnonymousAccess ? " (anonymous access allowed)" : "" ) );
}

server_peer::~server_peer() {
    Log::info( cc::note( "LocalNMA peer" ) + cc::debug( " " ) + getShortPeerDescription( true ) +
               cc::debug( " instance is " ) + cc::warn( "destroyed" ) );
}

server_peer::mutex_type& server_peer::peer_mtx() const {
    // return peer_mutex_;
    return g_mtx_for_set_all_peers;
}

tiny::dispatch::queue_id_t server_peer::get_dispatch_queue_id() const {
    if( dispatch_queue_id_.empty() )
        dispatch_queue_id_ = std::string( "micro NMA peer queue " ) + unique_string_identifier();
    return dispatch_queue_id_;
}

std::string server_peer::getRemoteIp() const {
    lock_type lock( peer_mtx() );
    if( cached_remote_ip_.empty() )
        cached_remote_ip_ = server_peer_base::getRemoteIp();
    return cached_remote_ip_;
}
std::string server_peer::getOrigin() const {
    lock_type lock( peer_mtx() );
    if( cached_origin_.empty() )
        cached_origin_ = server_peer_base::getRemoteIp();
    return cached_origin_;
}

const std::string& server_peer::userId() const {  // if not empty, then logged in
    return strUserId_;
}
const std::string& server_peer::uiType() const {
    return strTypeOfUI_;
}

bool server_peer::stat_peer_register( server_peer* pPeer ) {
    if( !pPeer )
        return false;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itFind = g_set_all_peers.find( pPeer ), itEnd = g_set_all_peers.end();
    if( itFind != itEnd )
        return false;
    g_set_all_peers.insert( pPeer );
    pPeer->ref_retain();  // manual ref management
    return true;
}
bool server_peer::stat_peer_unregister( server_peer* pPeer ) {
    if( !pPeer )
        return false;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itFind = g_set_all_peers.find( pPeer ), itEnd = g_set_all_peers.end();
    if( itFind == itEnd )
        return false;
    g_set_all_peers.erase( pPeer );
    pPeer->ref_release();  // manual ref management
    return true;
}
bool server_peer::stat_peer_is_registered( server_peer* pPeer ) {
    if( !pPeer )
        return false;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itFind = g_set_all_peers.find( pPeer ), itEnd = g_set_all_peers.end();
    if( itFind == itEnd )
        return false;
    return true;
}
size_t server_peer::stat_locked_for_each_peer( server_peer::fn_for_each_peer_t fn ) {
    if( !fn )
        return 0;
    size_t cnt = 0;
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    set_peers_t::iterator itWalk = g_set_all_peers.begin(), itEnd = g_set_all_peers.end();
    for( ; itWalk != itEnd; ++itWalk ) {
        server_peer* pPeer = ( *itWalk );
        if( !fn( pPeer ) )
            break;
        ++cnt;
    }
    return cnt;
}

void server_peer::stats_info::clear() {
    cntPeers_ = 0;
    cntUnknown_ = 0;
    cntUIs_ = 0;
    map_peer_descriptions_.clear();
}
std::string server_peer::stats_info::get_log_string() const {
    size_t cntSum = sum();
    std::stringstream ss;
    size_t cntItemsWritten = 0;
    if( cntSum > 0 ) {
        if( cntItemsWritten > 0 )
            ss << cc::debug( ", " );
        ss << cc::info( "sum" ) << cc::debug( "=" ) << cc::size10( cntSum );
        ++cntItemsWritten;
    }
    if( cntUnknown_ > 0 ) {
        if( cntItemsWritten > 0 )
            ss << cc::debug( ", " );
        ss << cc::info( "unknown" ) << cc::debug( "=" ) << cc::size10( cntUnknown_ );
        ++cntItemsWritten;
    }
    if( cntUIs_ > 0 ) {
        if( cntItemsWritten > 0 )
            ss << cc::debug( ", " );
        ss << cc::info( "UI" ) << cc::debug( "=" ) << cc::size10( cntUIs_ );
        ++cntItemsWritten;
    }
    return ss.str();
}
void server_peer::stat_peer_stats( server_peer::stats_info& si, bool isColored ) {
    si.clear();
    lock_type_set_all_peers lock( g_mtx_for_set_all_peers );
    si.cntPeers_ = g_set_all_peers.size();
    set_peers_t::const_iterator itWalk = g_set_all_peers.cbegin(), itEnd = g_set_all_peers.cend();
    for( ; itWalk != itEnd; ++itWalk ) {
        const server_peer* pPeer = ( *itWalk );
        std::string strDesc = pPeer->getShortPeerDescription( isColored );
        bool bIsLoggedIn = pPeer->isLoggedIn();
        std::string strPeerType;
        if( bIsLoggedIn ) {
            strPeerType = "UI";
            ++si.cntUIs_;
        }
        else {
            strPeerType = "Unknown";
            ++si.cntUnknown_;
        }
        si.map_peer_descriptions_[ strPeerType ].insert( strDesc );
    }  // for( ; itWalk != itEnd; ++itWalk )
}
void server_peer::log_extended_peer_statistics( tiny::ws::server::server* pSrv /*= nullptr*/ ) {
    if( !g_bEnableMicroNmaPeerStatsLogging )
        return;
    server_peer::stats_info si;
    server_peer::stat_peer_stats( si, true );
    std::stringstream ss;
    static const char g_strLongSeparator[] = "========================================================================="
                                             "===========================================================";
    static const char strLeadingIndent[] = "    ";
    std::string strLogSummary = si.get_log_string();
    if( strLogSummary.empty() )
        strLogSummary = cc::warn( "there are no connections at all" );
    ss << "\n" << cc::debug( g_strLongSeparator );
    if( pSrv ) {
        ss << "\n"
           << strLeadingIndent << cc::info( "Micro NMA server" ) << cc::debug( " summary traffic stats: " )
           << pSrv->getLifeTimeDescription( true ) << cc::debug( ", " ) << pSrv->getTrafficStatsDescription( true );
        //
        std::string strEventEPSs = pSrv->getEventStatsDescription( true );
        if( !strEventEPSs.empty() )
            ss << "\n"
               << strLeadingIndent << cc::info( "Micro NMA server" ) << cc::debug( " summary event stats: " )
               << cc::debug( ", " ) << strEventEPSs;
    }
    ss << "\n" << strLeadingIndent << cc::debug( "Peer stats: " ) + strLogSummary;
    static const char g_str_indent[] = "    ";
    server_peer::stats_info::map_peer_descriptions_t::const_iterator itPeerTypesWalk =
                                                                         si.map_peer_descriptions_.cbegin(),
                                                                     itPeerTypesEnd = si.map_peer_descriptions_.cend();
    for( ; itPeerTypesWalk != itPeerTypesEnd; ++itPeerTypesWalk ) {
        std::string strPeerType = itPeerTypesWalk->first;
        const server_peer::stats_info::peer_descriptions_t& ref_peer_descriptions = itPeerTypesWalk->second;
        size_t cnt = ref_peer_descriptions.size();
        if( cnt == 0 )
            continue;
        ss << "\n"
           << strLeadingIndent << g_str_indent << cc::size10( cnt ) << cc::debug( " " ) << cc::info( strPeerType )
           << cc::debug( tiny::tools::format( " peer%s", ( cnt > 1 ) ? "s" : "" ) );
        server_peer::stats_info::peer_descriptions_t::const_iterator itDescWalk = ref_peer_descriptions.cbegin(),
                                                                     itDescEnd = ref_peer_descriptions.cend();
        for( ; itDescWalk != itDescEnd; ++itDescWalk ) {
            const std::string& strDesc = ( *itDescWalk );
            ss << "\n" << strLeadingIndent << g_str_indent << g_str_indent << strDesc;
        }  // for( ; itDescWalk != itDescEnd; ++itDescWalk )
    }      // for( ; itPeerTypesWalk != itPeerTypesEnd; ++itPeerTypesWalk )
    ss << "\n" << cc::debug( g_strLongSeparator );
    Log::info( ss.str() );
}

std::string server_peer::getShortTypeDescrition( bool isColored /*= false*/ ) const {
    // lock_type lock( peer_mtx() );
    bool bIsLoggedIn = isLoggedIn();
    std::string strSep( "/" ), strBraceOpen( "(" ), strBraceClose( ")" ), strType;
    if( !bIsLoggedIn ) {
        strType = "plain-micro-nma-peer";
        if( isColored ) {
            if( !strType.empty() )
                strType = cc::error( strType );
        }
    }
    else {
        strType = uiType();
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
    std::string strUserId = userId();
    if( isColored ) {
        if( !strUserId.empty() )
            strUserId = cc::note( strUserId );
    }
    ss << strType;
    if( !strUserId.empty() )
        ss << strSep << strUserId;
    return ss.str();
}
std::string server_peer::getShortPeerDescription( bool isColored /*= false*/ ) const {
    // lock_type lock( peer_mtx() );
    return server_peer_base::getShortPeerDescription( isColored );
}

void server_peer::onPeerRegister() {
    std::string strPeerDescription = getShortPeerDescription( true );
    server_peer_base::onPeerRegister();  // traffic stats
    stat_peer_register( make_ptr( this ) );
    Log::debug( cc::debug( "Registered peer " ) + strPeerDescription );
    log_extended_peer_statistics( (tiny::ws::server::server*) ( &( srv() ) ) );
}
void server_peer::onPeerUnregister() {  // peer will no longer receive onMessage after call to this
    std::string strPeerDescription = getShortPeerDescription( true );
    server_peer_base::onPeerUnregister();  // traffic stats
    /* Should it be implemented in micro_nma_peer? */
    // onHandleDestruction();
    stat_peer_unregister( make_ptr( this ) );
    Log::debug( cc::debug( "Unregistered peer " ) + strPeerDescription );
    log_extended_peer_statistics( (tiny::ws::server::server*) ( &( srv() ) ) );
    if( isLoggedIn() && uiType() == "HUB" ) {
        if( auto clusterManagerPtr = HubPlatform::Uid::ClusterManager::getInstance() ) {
            clusterManagerPtr->onExternalControllerLoggedOut( userId() );
        }
        else {
            Log::error( cc::error( "Cluster manager is not initialized" ) );
        }
    }
}

bool server_peer::isLoggedIn() const {
    lock_type lock( peer_mtx() );
    bool bIsLoggedIn = !userId().empty();
    return bIsLoggedIn;
}

void server_peer::onLoginConfirmed( const std::string& userId, bool isUser ) {
    lock_type lock( peer_mtx() );
    strUserId_ = userId;
    strTypeOfUI_ = isUser ? "UI" : "HUB";
    // NOTE: remove timer to disconnect peer as it's logged in
    lws_set_timer_usecs( hdl_, LWS_SET_TIMER_USEC_CANCEL );
    Log::info( cc::debug( isUser ? "User " : "Controller " ) + cc::note( strUserId_ ) +
               cc::debug( " is successfully logged in local/offline mode " ) );
    log_extended_peer_statistics( (tiny::ws::server::server*) ( &( srv() ) ) );
}

bool server_peer::sendMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( eOpCode != tiny::ws::server::opcv::text )
        return false;
    if( !isConnected() )
        return false;
    if( eOpCode != tiny::ws::server::opcv::text || ( !tiny::ws::utils::is_ping_or_pong( msg ) ) )
        log_extended_peer_statistics( (tiny::ws::server::server*) ( &( srv() ) ) );
    if( srv().log_ws_rx_tx_ ) {
        if( eOpCode != tiny::ws::server::opcv::text ||
            ( !tiny::ws::utils::is_ping_or_pong( msg ) ) )  // do not log ping-pong
            Log::debug( cc::rpc_log( cc::e_rpc_log_type_t::erpcltt_ws_tx,
                                     std::string( "LocalNMA" ),
                                     getShortPeerDescription( true ),
                                     msg,
                                     ( eOpCode == tiny::ws::server::opcv::binary ) ? true : false ) );
    }
    return server_peer_base::sendMessage( msg, eOpCode );
}
bool server_peer::postMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( g_bSingleThreadedPeerAnswers )
        return sendMessage( msg, eOpCode );
    if( !isConnected() )
        return false;
    server_peer* pPeer = server_peer::make_ptr( this );
    pPeer->ref_retain();  // manual ref management
    try {
        tiny::dispatch::async( get_dispatch_queue_id(),
                               std::bind(
                                   []( server_peer* pPeer, std::string msg, tiny::ws::server::opcv eOpCode ) -> void {
                                       try {
                                           pPeer->sendMessage( msg, eOpCode );
                                       }
                                       catch( ... ) {
                                       }
                                       pPeer->ref_release();  // manual ref management
                                   },
                                   server_peer::make_ptr( this ),
                                   msg,
                                   eOpCode ) );
        return true;
    }
    catch( const std::exception& e ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( e.what() ) +
                    cc::error( ") while submitting message to async dispatcher (LocalNMA peer), message " ) +
                    cc::j( msg ) );
    }
    catch( ... ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( "unknown" ) +
                    cc::error( ") while submitting message to async dispatcher (LocalNMA peer) message " ) +
                    cc::j( msg ) );
    }
    return false;
}

bool server_peer::sendMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( eOpCode != tiny::ws::server::opcv::text )
        return false;
    if( !isConnected() )
        return false;
    return server_peer_base::sendMessage( msg, eOpCode );
}
bool server_peer::postMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    if( g_bSingleThreadedPeerAnswers )
        return sendMessageNoLog( msg, eOpCode );
    if( !isConnected() )
        return false;
    server_peer* pPeer = server_peer::make_ptr( this );
    pPeer->ref_retain();  // manual ref management
    try {
        tiny::dispatch::async( get_dispatch_queue_id(),
                               std::bind(
                                   []( server_peer* pPeer, std::string msg, tiny::ws::server::opcv eOpCode ) -> void {
                                       try {
                                           pPeer->sendMessageNoLog( msg, eOpCode );
                                       }
                                       catch( ... ) {
                                       }
                                       pPeer->ref_release();  // manual ref management
                                   },
                                   server_peer::make_ptr( this ),
                                   msg,
                                   eOpCode ) );
        return true;
    }
    catch( const std::exception& e ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( e.what() ) +
                    cc::error( ") while submitting message to async dispatcher (LocalNMA peer, NoLog), message " ) +
                    cc::j( msg ) );
    }
    catch( ... ) {
        pPeer->ref_release();  // manual ref management
        Log::fatal( cc::error( "exception (" ) + cc::warn( "unknown" ) +
                    cc::error( ") while submitting message to async dispatcher (LocalNMA peer, NoLog) message " ) +
                    cc::j( msg ) );
    }
    return false;
}

void server_peer::onMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) {
    server_peer_base::onMessage( msg, eOpCode );  // traffic stats
    if( eOpCode != tiny::ws::server::opcv::text || ( !tiny::ws::utils::is_ping_or_pong( msg ) ) )
        log_extended_peer_statistics( (tiny::ws::server::server*) ( &( srv() ) ) );
    if( eOpCode != tiny::ws::server::opcv::text ) {
        Log::error( cc::error( "Ignoring non-text message got by MicroNmaPeer " ) + getShortPeerDescription( true ) );
        return;
    }
    if( srv().log_ws_rx_tx_ ) {
        if( eOpCode != tiny::ws::server::opcv::text ||
            ( !tiny::ws::utils::is_ping_or_pong( msg ) ) )  // do not log ping-pong
            Log::debug( cc::rpc_log( cc::e_rpc_log_type_t::erpcltt_ws_rx,
                                     std::string( "LocalNMA" ),
                                     getShortPeerDescription( true ),
                                     msg,
                                     ( eOpCode == tiny::ws::server::opcv::binary ) ? true : false ) );
    }
    try {
        auto fnHandler = []( server_peer& ws, std::string msg ) {
            try {
                JsonObject joMessage;
                if( !parseJson( msg, joMessage ) ) {
                    Log::error( cc::error( "Signal web socket got bad JSON: " ) + cc::j( msg ) );
                    return;
                }
                server_peer_message serverMessage( &ws );
                serverMessage.setMessage( joMessage );
                serverMessage.answerPrepare();
                server_proto serverProto;
                std::string strMethod = serverMessage.getMethod();
                if( ( ws.isLoggedIn() || Db::getFlag( OfflineAnonymousAccessFlagName ) ||
                      serverProto.allowEveryone( strMethod ) ) &&
                    !serverProto.has( strMethod ) ) {
                    ws.on_message( serverMessage );
                }
                else {
                    serverProto.processCmd( serverMessage );
                }
                serverMessage.postAnswer( true );
            }
            catch( ... ) {
            }
            ws.ref_release();  // manual ref management
        };
        ref_retain();  // manual ref management
        if( server_peer::g_bSingleThreadedPeerMessages )
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
        stat_handle_async_mechanism_error( *this, msg, s1, s2 );
    }
    catch( ... ) {
        ref_release();  // manual ref management
        std::string s1 = cc::error( "exception (" ) + cc::warn( "unknown" ) +
                         cc::error( ") while submitting message to async dispatcher, message " ) + cc::j( msg );
        std::string s2 = std::string( "exception (" ) + std::string( "unknown" ) +
                         std::string( ") while submitting message to async dispatcher, message " ) + msg;
        stat_handle_async_mechanism_error( *this, msg, s1, s2 );
    }
}
