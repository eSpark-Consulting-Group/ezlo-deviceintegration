server::server()
: server_serial_number_( 0 )
, reuse_addr_( true )
, listen_backlog_( 0 ) {
    traffic_stats::registrer_default_event_queues_for_web_socket_server();
    api_.onConnect_ = [this]( connection_identifier_t cid,
                              struct lws* /*wsi*/,
                              const char* /*strPeerClientAddressName*/,
                              const char* /*strPeerRemoteIP*/ ) { onOpen( cid ); };
    api_.onDisconnect_ = [this]( connection_identifier_t cid, const std::string& strMessage ) {
        onClose( cid, strMessage.c_str(), 0, "" );
    };
    api_.onMessage_ = [this]( connection_identifier_t cid, const message_payload_data& data ) {
        opcv eOpCode = ( data.type() == LWS_WRITE_BINARY ) ? opcv::binary : opcv::text;
        std::string s;
        data.store_to_string( s );
        onMessage( cid, eOpCode, s );
    };
    api_.onHttp_ = [this]( connection_identifier_t cid ) {
        onHttp( cid );  // return value is ignored in this implementation
    };
    api_.onFail_ = [this]( connection_identifier_t cid, const std::string& strMessage ) {
        if( !strMessage.empty() )
            onLogMessage( e_ws_log_message_type_t::eWSLMT_error, cc::error( strMessage ) );
        onFail( cid );
    };
    api_.onLogMessage_ = [this]( e_ws_log_message_type_t eWSLMT, const std::string& strMessage ) {
        onLogMessage( eWSLMT, strMessage );
    };
}
server::~server() {
    close();
}
nlohmann::json server::toJSON( bool bSkipEmptyStats /*= true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "server";
    jo[ "server_type" ] = cc::strip( type() );
    jo[ "uid" ] = "sum";
    jo[ "remote_ip" ] = "";
    jo[ "nma_peer_type" ] = "NMA server";
    jo[ "nma_hub_type" ] = "";
    jo[ "nma_ui_type" ] = "";
    jo[ "serial" ] = "";
    jo[ "email" ] = "";
    jo[ "portal_monitoring" ] = "";
    jo[ "port" ] = port();
    jo[ "scheme" ] = last_scheme_cached_;
    jo[ "stats" ] = traffic_stats::toJSON( bSkipEmptyStats );
    return jo;
}
size_t server::request_new_peer_serial_number() {
    size_t n = server_serial_number_;
    ++server_serial_number_;
    return n;
}

bool server::isServerSide() const {
    return true;
}
std::string server::type() const {
    return api_.use_ssl_ ? cc::success( "SSL/TLS" ) : cc::fatal( "non-secure" );
}
int server::port() const {
    return api_.ctx_info_.port;
}
int server::defaultPort() const {
    return api_.use_ssl_ ? 443 : 9666;
}
int server::getNumberOfConnections() const {
    return api_.getNumberOfConnections();
}
bool server::open( const std::string& scheme, int nPort, const lws_context_configuration& configuration ) {
    return open_autofind_port( scheme, nPort, configuration );
}

bool server::appendVHost( server_vhost_settings& settings ) {
    if( !api_.appendVHost( settings ) ) {
        traffic_stats::event_add( g_strEventNameWebSocketServerStartFail );
        return false;
    }
    traffic_stats::log_open();
    traffic_stats::event_add( g_strEventNameWebSocketServerStart );
    return true;
}

// if nPort != 0 ==> usual open, no autofind
bool server::open_autofind_port( const std::string& scheme,
                                 int& nPort,
                                 const lws_context_configuration& configuration ) {
    last_scheme_cached_ = scheme;
    bool isSSL = ( scheme == "ws" ) ? false : true;
    basic_network_settings &bns_api = api_, bns_this = ( *this );
    bns_api = bns_this;
    if( !api_.init( isSSL, nPort, this, configuration ) ) {
        traffic_stats::event_add( g_strEventNameWebSocketServerStartFail );
        return false;
    }
    traffic_stats::log_open();
    traffic_stats::event_add( g_strEventNameWebSocketServerStart );
    return true;
}
void server::close() {
    server_api::lock_type lock( api_.mtx_api() );
    api_.deinit();
    traffic_stats::log_close();
    // server_serial_number_ = 0;
    traffic_stats::event_add( g_strEventNameWebSocketServerStop );
}
void server::close( hdl_t hdl, int nCloseStatus, const std::string& msg ) {
    api_.close( hdl, nCloseStatus, msg );
}
void server::cancel_hdl( hdl_t /*hdl*/ ) {
    // TO-FIX: cancel() not implemented yet
}
void server::pause_reading_hdl( hdl_t /*hdl*/ ) {
    // TO-FIX: pause_reading() not implemented yet
}
void server::poll( fn_continue_status_flag_t fnContinueStatusFlag ) {
    api_.poll( fnContinueStatusFlag );
}
void server::reset() {
    // TO-FIX: reset() not implemented yet
}
bool server::service_mode_supported() const {
    return api_.service_mode_supported();
}
void server::service_interrupt() {
    api_.service_interrupt();
}
void server::service( fn_continue_status_flag_t fnContinueStatusFlag ) {
    api_.service( fnContinueStatusFlag );
}
std::string server::getRemoteIp( hdl_t hdl ) {
    return api_.getPeerRemoteIP( hdl );  // notice: no port is returned
}
std::string server::getOrigin( hdl_t hdl ) {
    return api_.getPeerClientAddressName( hdl );  // notice: used as "origin"
}
bool server::sendMessage( hdl_t hdl, const std::string& msg, opcv eOpCode /*= opcv::text*/ ) {
    message_payload_data data;
    if( eOpCode == opcv::binary )
        data.set_binary( msg );
    else
        data.set_text( msg );
    if( !api_.send( hdl, data ) )
        return false;
    if( eOpCode == opcv::text ) {
        traffic_stats::log_text_tx( msg.length() );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesSentText );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesSent );
    }
    else if( eOpCode == opcv::binary ) {
        traffic_stats::log_bin_tx( msg.size() );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesSentBinary );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesSent );
    }
    return true;
}
//
peer_ptr_t server::onPeerInstantiate( hdl_t hdl ) {
    peer_ptr_t pPeer( nullptr );
    try {
        if( onPeerInstantiate_ )
            pPeer = onPeerInstantiate_( *this, hdl );
        if( !pPeer )
            pPeer = new peer( *this, hdl );
    }
    catch( ... ) {
    }
    return pPeer;
}
peer_ptr_t server::getPeer( hdl_t hdl ) {
    return api_.getPeer( hdl );
}
peer_ptr_t server::detachPeer( hdl_t hdl ) {
    return api_.detachPeer( hdl );
}
bool server::onPeerRegister( peer_ptr_t pPeer ) {
    if( !pPeer )
        return false;
    pPeer->opened_ = true;
    hdl_t hdl = pPeer->hdl();
    if( onPeerRegister_ )
        onPeerRegister_( pPeer );
    pPeer->onPeerRegister();
    pPeer->ref_retain();
    traffic_stats::event_add( g_strEventNameWebSocketPeerConnect );
    return true;
}
bool server::onPeerUnregister( peer_ptr_t pPeer ) {
    if( !pPeer )
        return false;
    traffic_stats::event_add( g_strEventNameWebSocketPeerDisconnect );
    if( onPeerUnregister_ )
        onPeerUnregister_( pPeer );
    pPeer->onPeerUnregister();
    pPeer->opened_ = false;
    pPeer->ref_release();
    return true;
}
int64_t server::delayed_adjustment_pong_timeout( connection_identifier_t cid ) const {  // NLWS-specific
    return api_.delayed_adjustment_pong_timeout( cid );
}
void server::delayed_adjustment_pong_timeout( connection_identifier_t cid, int64_t to ) {  // NLWS-specific
    api_.delayed_adjustment_pong_timeout( cid, to );
}

//
void server::onOpen( hdl_t hdl ) {
    basic_socket::onOpen( hdl );
    peer_ptr_t pPeer( nullptr );
    try {
        pPeer = onPeerInstantiate( hdl );
    }
    catch( ... ) {
        pPeer = nullptr;
    }
    if( !pPeer ) {
        std::string strRemoteIp = getRemoteIp( hdl );
        std::stringstream ss;
        ss << cc::error( "Failed to instantiate peer(" ) << cc::u( strRemoteIp ) << cc::error( ")" );
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
        close( hdl, close_status::internal_endpoint_error, "internal peer initialization error" );
        return;
    }
    onPeerRegister( pPeer );
    size_t x = pPeer->ref_release();  // un-needed ref after instantiate
    if( x > 0 )
        api_.setPeer( hdl, pPeer );
}
void server::onClose( hdl_t hdl,
                      const std::string& reason,
                      int local_close_code,
                      const std::string& local_close_code_as_str ) {
    basic_socket::onClose( hdl, reason, local_close_code, local_close_code_as_str );
    peer_ptr_t pPeer = detachPeer( hdl );
    if( pPeer ) {
        pPeer->was_disconnected_ = true;
        onPeerUnregister( pPeer );
    }
}
void server::onFail( hdl_t hdl ) {
    traffic_stats::event_add( traffic_stats::g_strEventNameWebSocketFail );
    basic_socket::onFail( hdl );
}
void server::onMessage( hdl_t hdl, opcv eOpCode, const std::string& msg ) {
    peer_ptr_t pPeer = getPeer( hdl );
    if( pPeer ) {
        pPeer->onMessage( msg, eOpCode );
        if( eOpCode == opcv::text ) {
            traffic_stats::log_text_rx( msg.length() );
            traffic_stats::event_add( g_strEventNameWebSocketMessagesRecvText );
            traffic_stats::event_add( g_strEventNameWebSocketMessagesRecv );
        }
        else if( eOpCode == opcv::binary ) {
            traffic_stats::log_bin_rx( msg.size() );
            traffic_stats::event_add( g_strEventNameWebSocketMessagesRecvBinary );
            traffic_stats::event_add( g_strEventNameWebSocketMessagesRecv );
        }
        return;
    }
    //
    std::string strRemoteIp = getRemoteIp( hdl );
    std::stringstream ss;
    ss << cc::warn( "No instantiated peer(" ) << cc::u( strRemoteIp ) << cc::warn( ") for message (" ) << cc::str( msg )
       << cc::warn( "<<" );
    onLogMessage( e_ws_log_message_type_t::eWSLMT_warning, ss.str() );
    //
    basic_socket::onMessage( hdl, eOpCode, msg );
    if( eOpCode == opcv::text ) {
        traffic_stats::log_text_rx( msg.length() );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesRecvText );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesRecv );
    }
    else if( eOpCode == opcv::binary ) {
        traffic_stats::log_bin_rx( msg.size() );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesRecvBinary );
        traffic_stats::event_add( g_strEventNameWebSocketMessagesRecv );
    }
}
bool server::onHttp( hdl_t hdl ) {
    return onHttp_ ? onHttp_( *this, hdl ) : false;
}

// void server::onStreamSocketInit( int native_fd ) {
//	basic_socket::onStreamSocketInit( native_fd );
//}
void server::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    basic_socket::onLogMessage( eWSLMT, msg );
}
const security_args& server::onGetSecurityArgs() const {
    return ( *this );
}
void server::onBeforeCreateContext( onBeforeCreateContext_t handler ) {
    api_.onBeforeCreateContext( std::move( handler ) );
}
