client::client( const std::string& serverUri )
: m_serverURI( serverUri )
, m_isRestartTimerEnabled( new std::atomic_bool( true ) ) {
    traffic_stats::registrer_default_event_queues_for_web_socket_client();
    m_clientApi.onConnect_ = [this]() {
        onOpen( m_clientApi.cid_ );
        setConnected( true );
    };
    m_clientApi.onDisconnect_ = [this]( const std::string& strMessage ) {
        setConnected( false );
        onClose( m_clientApi.cid_, strMessage.c_str(), 0, "" );
    };
    m_clientApi.onMessage_ = [this]( const message_payload_data& data ) {
        opcv eOpCode = ( data.type() == LWS_WRITE_BINARY ) ? opcv::binary : opcv::text;
        std::string s;
        data.store_to_string( s );
        onMessage( m_clientApi.cid_, eOpCode, s );
    };
    m_clientApi.onFail_ = [this]( const std::string& strMessage ) {
        if( !strMessage.empty() )
            onLogMessage( e_ws_log_message_type_t::eWSLMT_error, cc::error( strMessage ) );
        setConnected( false );
        onFail( m_clientApi.cid_ );
    };
    m_clientApi.onLogMessage_ = [this]( e_ws_log_message_type_t eWSLMT, const std::string& strMessage ) {
        onLogMessage( eWSLMT, strMessage );
    };
    m_clientApi.onDelayDeinit_ = [this]() { onDelayDeinit(); };
}
client::~client() {
    enableRestartTimer( false );
    close();
}

void client::resetServerUrl() {
    m_serverURI.clear();
}

nlohmann::json client::toJSON( bool bSkipEmptyStats /*= true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "client";
    jo[ "url" ] = uri();
    jo[ "stats" ] = traffic_stats::toJSON( bSkipEmptyStats );
    return jo;
}
bool client::isServerSide() const {
    return false;
}

std::string client::type() const {
    return m_clientApi.ssl_flags_ ? cc::success( "SSL/TLS" ) : cc::fatal( "non-secure" );
}
std::string client::uri() const {
    return m_clientApi.strURL_;
}

const std::string& client::serverUri() const {
    return m_serverURI;
}

void client::validateOrganizationName( std::string organizationName ) {
    m_clientApi.validOrganizationName_ = std::move( organizationName );
}

void client::validateOrganizationalUnitName( std::string organizationalUnitName ) {
    m_clientApi.validOrganizationalUnitName_ = std::move( organizationalUnitName );
}

void client::validateCommonName( std::string commonName ) {
    m_clientApi.validCommonName_ = std::move( commonName );
}

void client::validateHostName( bool validate ) {
    m_clientApi.ssl_client_skip_host_name_check_ = !validate;
}

bool client::open( const std::string& uri ) {
    close();
    try {
        m_serverURI = uri;
        basic_network_settings &bns_api = m_clientApi, bns_this = ( *this );
        bns_api = bns_this;
        if( !m_clientApi.init( uri, this ) ) {
            traffic_stats::event_add( g_strEventNameWebSocketClientConnectFail );
            return false;
        }
        traffic_stats::log_open();
        traffic_stats::event_add( g_strEventNameWebSocketClientConnect );
        return true;
    }
    catch( const std::exception& ex ) {
        const char* strWhat = ex.what();
        if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
            strWhat = "unknown exception";
        std::stringstream ss;
        ss << cc::error( "open: " ) << cc::warn( strWhat );
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
    }
    catch( ... ) {
        std::stringstream ss;
        ss << cc::error( "open: " ) << cc::warn( "unknown exception" );
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
    }
    traffic_stats::event_add( g_strEventNameWebSocketClientConnectFail );
    return false;
}
bool client::openLocalHost( int nPort ) {
    std::stringstream ss;
    ss << "ws://localhost:" << nPort;
    std::string uri = ss.str();
    return open( uri );
}
void client::close() {
    if( !isRestartTimerEnabled() ) {
        std::lock_guard< std::mutex > lock( m_connectionRestartMutex );
        m_connectionRestartTimer.stop();
    }
    m_clientApi.deinit();
    traffic_stats::log_close();
    traffic_stats::event_add( g_strEventNameWebSocketClientDisconnect );
}
void client::closeConnection() {
    enableRestartTimer( false );
    close();
}
void client::restoreConnection() {
    enableRestartTimer( true );
}
void client::async_close( const std::string& msg,
                          int nCloseStatus,            // = int(close_status::going_away)
                          size_t nTimeoutMilliseconds  // = 0
) {
    ref_retain();
    try {
        std::thread( [this, msg, nCloseStatus, nTimeoutMilliseconds]() -> void {
            try {
                if( nTimeoutMilliseconds > 0 )
                    std::this_thread::sleep_for( std::chrono::milliseconds( nTimeoutMilliseconds ) );
                close( msg, nCloseStatus );
            }
            catch( ... ) {
            }
            ref_release();
        } )
            .detach();
        return;
    }
    catch( ... ) {
        ref_release();
    }
}
void client::close( const std::string& msg, int nCloseStatus /*= int(close_status::going_away)*/ ) {
    m_clientApi.close( nCloseStatus, msg );
    // m_clientApi.deinit();
    traffic_stats::log_close();
    traffic_stats::event_add( g_strEventNameWebSocketClientDisconnect );
}
void client::cancel() {
    // TO-FIX: cancel() not yet implemented
}
void client::pause_reading() {
    // TO-FIX: pause_reading() not yet implemented
}

bool client::isConnected() const noexcept {
    return m_clientApi.connection_flag_;
}
void client::setConnected( bool /*state*/ ) {
    // notice: "state" parameter must be in sync with m_clientApi.connection_flag_
}

bool client::sendMessage( const std::string& msg, opcv eOpCode ) {
    message_payload_data data;
    if( eOpCode == opcv::binary )
        data.set_binary( msg );
    else
        data.set_text( msg );
    if( !m_clientApi.send( data ) )
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
void client::onMessage( hdl_t hdl, opcv eOpCode, const std::string& msg ) {
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
void client::onDisconnected() {
    if( m_onDisconnected )
        m_onDisconnected( *this );
    traffic_stats::log_close();
}
void client::onOpen( hdl_t hdl ) {
    basic_socket::onOpen( hdl );
    // sendMessage( Proto::getRegisterMethod() );
}

void client::onRestartConnectionTimer() {
    onLogMessage( e_ws_log_message_type_t::eWSLMT_info, "NLWS: will re-connect to " + cc::u( m_serverURI ) );
    close();
    open( m_serverURI );
}

void client::impl_ensure_restart_timer_is_running() {
    if( !isRestartTimerEnabled() )
        return;
    // if( isConnected() )
    //	return;
    std::lock_guard< std::mutex > lock( m_connectionRestartMutex );
    if( m_connectionRestartTimer.running() )
        return;
    auto enabled = m_isRestartTimerEnabled;
    m_connectionRestartTimer.run( std::chrono::seconds( timeout_restart_on_close_ ), [enabled, this]() {
        std::thread( [enabled, this]() {
            if( *enabled ) {
                onRestartConnectionTimer();
            }
        } )
            .detach();
    } );
}
void client::onClose( hdl_t hdl,
                      const std::string& reason,
                      int local_close_code,
                      const std::string& local_close_code_as_str ) {
    basic_socket::onClose( hdl, reason, local_close_code, local_close_code_as_str );
    impl_ensure_restart_timer_is_running();
    traffic_stats::log_close();
    traffic_stats::event_add( g_strEventNameWebSocketClientDisconnect );
}
void client::onFail( hdl_t hdl ) {
    traffic_stats::event_add( traffic_stats::g_strEventNameWebSocketFail );
    basic_socket::onFail( hdl );
    impl_ensure_restart_timer_is_running();
    traffic_stats::log_close();
}
void client::onDelayDeinit() {  // NLWS-specific
}
// void client::onStreamSocketInit( int native_fd ) {
//	basic_socket::onStreamSocketInit( native_fd );
//}
void client::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    basic_socket::onLogMessage( eWSLMT, msg );
}
const security_args& client::onGetSecurityArgs() const {
    return ( *this );
}
bool client::isRestartTimerEnabled() const {
    return *m_isRestartTimerEnabled;
}
void client::enableRestartTimer( bool isEnabled ) {
    *m_isRestartTimerEnabled = isEnabled;
    if( m_isRestartTimerEnabled )
        impl_ensure_restart_timer_is_running();
    else {
        std::lock_guard< std::mutex > lock( m_connectionRestartMutex );
        m_connectionRestartTimer.stop();
    }
}
int64_t client::delayed_adjustment_pong_timeout() const {  // NLWS-specific
    return m_clientApi.delayed_adjustment_pong_timeout_;
}
void client::delayed_adjustment_pong_timeout( int64_t to ) {  // NLWS-specific
    m_clientApi.delayed_adjustment_pong_timeout_ = to;
}

void client::setClientBufferSizeRx( size_t newSize ) {
    basic_api::g_nDefaultClientBufferSizeRX = newSize;
}
