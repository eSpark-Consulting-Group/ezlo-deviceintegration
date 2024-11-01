peer::peer( server& srv, const hdl_t& hdl )
: srv_( srv )
, peer_serial_number_( srv.request_new_peer_serial_number() )
, hdl_( hdl )
, cid_( 0 )
, was_disconnected_( false ) {
    traffic_stats::registrer_default_event_queues_for_web_socket_peer();
    cid_ = stat_getCid( hdl );
}
peer::~peer() {
    if( !was_disconnected_ )
        close( "BYE!" );
}
nlohmann::json peer::toJSON( bool bSkipEmptyStats /*= true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "peer";
    jo[ "uid" ] = unique_string_identifier();
    jo[ "type_description" ] = getShortTypeDescrition( false );
    jo[ "description" ] = getShortPeerDescription( false );
    jo[ "remote_ip" ] = getRemoteIp();
    jo[ "connection_id" ] = getCidString();
    jo[ "serial_number" ] = serial_number();
    jo[ "scheme" ] = srv().last_scheme_cached_;
    jo[ "stats" ] = traffic_stats::toJSON( bSkipEmptyStats );
    return jo;
}
std::string peer::getShortTypeDescrition( bool isColored /*= false*/ ) const {
    // lock_type lock( ref_mtx() );
    std::string s( "peer" );
    if( isColored ) {
        if( !s.empty() )
            s = cc::info( s );
    }
    return s;
}
std::string peer::getShortPeerDescription( bool isColored /*= false*/ ) const {
    // lock_type lock( ref_mtx() );
    std::stringstream ss;
    std::string strSpace( " " ), strCommaSpace( ", " ), strIP( getRemoteIp() );
    if( isColored ) {
        strSpace = cc::debug( strSpace );
        strCommaSpace = cc::debug( strCommaSpace );
        if( !strIP.empty() )
            strIP = cc::u( strIP );
    }
    traffic_stats::time_point tpNow = traffic_stats::clock::now();
    ss << unique_string_identifier( isColored ) << strSpace << getShortTypeDescrition( isColored ) << strSpace << strIP
       << strCommaSpace << getLifeTimeDescription( tpNow, isColored ) << strCommaSpace
       << getTrafficStatsDescription( tpNow, isColored );
    return ss.str();
}
std::string peer::unique_string_identifier( bool isColored /*= false*/ ) const {
    std::string strCid( getCidString() ), strSlash( "/" );
    if( isColored ) {
        strSlash = cc::debug( strSlash );
        if( !strCid.empty() )
            strCid = cc::bright( strCid );
    }
    size_t s_no = serial_number();
    std::string strPeerSerialNumber = tiny::tools::format( "%" PRIu64, uint64_t( s_no ) );
    if( isColored )
        strPeerSerialNumber = cc::notice( strPeerSerialNumber );
    std::stringstream ss;
    ss << strCid << strSlash << strPeerSerialNumber;
    return ss.str();
}
void peer::onPeerRegister() {
    traffic_stats::log_open();
}
void peer::onPeerUnregister() {  // peer will no longer receive onMessage after call to this
    opened_ = false;
    traffic_stats::log_close();
}
bool peer::isServerSide() const {
    return true;
}
connection_identifier_t peer::cid() const {
    if( cid_ == 0 )
        cid_ = stat_getCid( hdl_ );
    return cid_;
}
connection_identifier_t peer::stat_getCid( const hdl_t& hdl ) {
    return hdl;
}

std::string peer::stat_getCidString( const connection_identifier_t& cid ) {
    std::string strCid;
    try {
        std::ostringstream ss;
        ss  //<< std::uppercase << std::setfill('0') << std::setw(8) << std::hex
            << cid;
        strCid = ss.str();
    }
    catch( ... ) {
        strCid.clear();
    }
    return strCid;
}
connection_identifier_t peer::stat_parseCid( const std::string& strCid ) {
    return reinterpret_cast< connection_identifier_t >( std::stol( strCid.c_str(), 0, 16 ) );
}
connection_identifier_t peer::stat_parseCid( const char* strCid ) {
    connection_identifier_t cid = 0;
    if( strCid != 0 && strCid[ 0 ] != '\0' ) {
        std::string s = strCid;
        cid = stat_parseCid( s );
    }
    return cid;
}

bool peer::isConnected() const noexcept {
    return opened_;
}
void peer::async_close( const std::string& msg,
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
void peer::close( const std::string& msg,
                  int nCloseStatus  // = int(close_status::going_away)
) {
    if( was_disconnected_ )
        return;
    ref_retain();
    try {
        srv_.close( hdl_, nCloseStatus, msg );
        traffic_stats::event_add( g_strEventNameWebSocketPeerDisconnect );
    }
    catch( const std::exception& ex ) {
        const char* strWhat = ex.what();
        if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
            strWhat = "unknown exception";
        std::stringstream ss;
        ss << cc::error( "Exception: " ) << cc::warn( strWhat );
        srv_.onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
        // clean_up( cid_ );
        traffic_stats::event_add( g_strEventNameWebSocketPeerDisconnectFail );
    }
    catch( ... ) {
        std::stringstream ss;
        ss << cc::error( "Unknown exception" );
        srv_.onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
        // clean_up( cid_ );
        traffic_stats::event_add( g_strEventNameWebSocketPeerDisconnectFail );
    }
    traffic_stats::log_close();
    ref_release();
}
void peer::cancel() {
    try {
        srv_.cancel_hdl( hdl_ );
    }
    catch( ... ) {
    }
}
void peer::pause_reading() {
    try {
        srv_.pause_reading_hdl( hdl_ );
    }
    catch( ... ) {
    }
}
void peer::onMessage( const std::string& msg, opcv eOpCode ) {
    if( onPeerMessage_ )
        onPeerMessage_( *this, msg, eOpCode );
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
void peer::onClose( const std::string& reason, int local_close_code, const std::string& local_close_code_as_str ) {
    opened_ = false;
    if( onPeerClose_ )
        onPeerClose_( *this, reason, local_close_code, local_close_code_as_str );
    traffic_stats::log_close();
}
void peer::onFail() {
    traffic_stats::event_add( traffic_stats::g_strEventNameWebSocketFail );
    srv().event_add( traffic_stats::g_strEventNameWebSocketFail );
    opened_ = false;
    if( onPeerFail_ )
        onPeerFail_( *this );
    traffic_stats::log_close();
}
bool peer::sendMessage( const std::string& msg, opcv eOpCode ) {
    // ss << cc::debug(">>> ") << cc::warn(getSender()) << cc::debug(", ") << cc::warn(getCidString()) << cc::debug(",
    // ") << cc::c(msg) << "/n";
    std::string strCid = getCidString();
    std::string strRemoteIp = getRemoteIp();
    try {
        if( !srv_.sendMessage( hdl_, msg, eOpCode ) )
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
    catch( const std::exception& ex ) {
        const char* strWhat = ex.what();
        if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
            strWhat = "unknown exception";
        std::stringstream ss;
        ss << cc::error( "nlws-peer sendMessage(" ) << cc::warn( strCid ) << cc::error( "," ) << cc::u( strRemoteIp )
           << cc::error( ") failed, exception: " ) << cc::warn( strWhat );
        srv_.onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
    }
    catch( ... ) {
        std::stringstream ss;
        ss << cc::error( "nlws-peer sendMessage(" ) << cc::warn( strCid ) << cc::error( "," ) << cc::u( strRemoteIp )
           << cc::error( ") failed, unknown exception" );
        srv_.onLogMessage( e_ws_log_message_type_t::eWSLMT_error, ss.str() );
    }
    return false;
}
void peer::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    srv_.onLogMessage( eWSLMT, msg );
}
const security_args& peer::onGetSecurityArgs() const {
    return srv_.onGetSecurityArgs();
}

std::string peer::getRemoteIp() const {
    return srv_.getRemoteIp( hdl_ );
}
std::string peer::getOrigin() const {
    return srv_.getOrigin( hdl_ );
}

std::string peer::getCidString() const {
    return stat_getCidString( cid_ );
}
