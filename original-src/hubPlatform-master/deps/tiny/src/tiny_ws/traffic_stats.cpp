traffic_stats::traffic_stats() {
    init();
}
traffic_stats::traffic_stats( traffic_stats::myrct x ) {
    init();
    assign( x );
}
traffic_stats::traffic_stats( traffic_stats::myrrt x ) {
    init();
    move( x );
}
traffic_stats::~traffic_stats() {
    clear();
}

traffic_stats::bytes_count_t traffic_stats::text_tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = text_tx_;
    return n;
}
traffic_stats::bytes_count_t traffic_stats::text_rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = text_rx_;
    return n;
}
traffic_stats::bytes_count_t traffic_stats::bin_tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = bin_tx_;
    return n;
}
traffic_stats::bytes_count_t traffic_stats::bin_rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = bin_rx_;
    return n;
}
traffic_stats::bytes_count_t traffic_stats::tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = text_tx_ + bin_tx_;
    return n;
}
traffic_stats::bytes_count_t traffic_stats::rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    bytes_count_t n = text_rx_ + bin_rx_;
    return n;
}

using namespace tiny::stats;
double traffic_stats::bps_text_tx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_text_tx_, tpNow );
    return lf;
}
double traffic_stats::bps_text_tx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_text_tx_ );
    return lf;
}
double traffic_stats::bps_text_tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_text_tx_ );
    return lf;
}

double traffic_stats::bps_text_rx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_text_rx_, tpNow );
    return lf;
}
double traffic_stats::bps_text_rx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_text_rx_ );
    return lf;
}
double traffic_stats::bps_text_rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_text_rx_ );
    return lf;
}

double traffic_stats::bps_bin_tx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_bin_tx_, tpNow );
    return lf;
}
double traffic_stats::bps_bin_tx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_bin_tx_ );
    return lf;
}
double traffic_stats::bps_bin_tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_bin_tx_ );
    return lf;
}

double traffic_stats::bps_bin_rx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_bin_rx_, tpNow );
    return lf;
}
double traffic_stats::bps_bin_rx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_bin_rx_ );
    return lf;
}
double traffic_stats::bps_bin_rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_bin_rx_ );
    return lf;
}

double traffic_stats::bps_tx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_all_tx_, tpNow );
    return lf;
}
double traffic_stats::bps_tx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_all_tx_ );
    return lf;
}
double traffic_stats::bps_tx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_all_tx_ );
    return lf;
}

double traffic_stats::bps_rx( time_point tpNow ) const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps( traffic_queue_all_rx_, tpNow );
    return lf;
}
double traffic_stats::bps_rx_last_known() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_last_known( traffic_queue_all_rx_ );
    return lf;
}
double traffic_stats::bps_rx() const {
    lock_type lock( const_cast< myrt >( *this ) );
    double lf = named_traffic_stats::stat_compute_bps_til_now( traffic_queue_all_rx_ );
    return lf;
}

traffic_stats::myrt traffic_stats::log_text_tx( bytes_count_t n ) {
    lock_type lock( *this );
    text_tx_ += n;
    traffic_record_item_t recNow( n );
    traffic_queue_all_tx_.push_back( recNow );
    traffic_queue_text_tx_.push_back( recNow );
    return ( *this );
}
traffic_stats::myrt traffic_stats::log_text_rx( traffic_stats::bytes_count_t n ) {
    lock_type lock( *this );
    text_rx_ += n;
    traffic_record_item_t recNow( n );
    traffic_queue_all_rx_.push_back( recNow );
    traffic_queue_text_rx_.push_back( recNow );
    return ( *this );
}
traffic_stats::myrt traffic_stats::log_bin_tx( traffic_stats::bytes_count_t n ) {
    lock_type lock( *this );
    bin_tx_ += n;
    traffic_record_item_t recNow( n );
    traffic_queue_all_tx_.push_back( recNow );
    traffic_queue_bin_tx_.push_back( recNow );
    return ( *this );
}
traffic_stats::myrt traffic_stats::log_bin_rx( traffic_stats::bytes_count_t n ) {
    lock_type lock( *this );
    bin_rx_ += n;
    traffic_record_item_t recNow( n );
    traffic_queue_all_rx_.push_back( recNow );
    traffic_queue_bin_rx_.push_back( recNow );
    return ( *this );
}

traffic_stats::e_last_instance_state_changing_type_t traffic_stats::last_instance_state_changing_type() const {
    lock_type lock( const_cast< myrt >( *this ) );
    e_last_instance_state_changing_type_t e = elisctt_;
    return e;
}
std::string traffic_stats::last_instance_state_changing_type_as_str() const {
    lock_type lock( const_cast< myrt >( *this ) );
    switch( elisctt_ ) {
    case elisctt_instantiated:
        return "instantiated";
    case elisctt_opened:
        return "opened";
    case elisctt_closed:
        return "closed";
    default:
        return "N/A-state";
    }
}
traffic_stats::time_point traffic_stats::instantiated() const {
    lock_type lock( const_cast< myrt >( *this ) );
    return time_stamp_instantiated_;
}
traffic_stats::nanoseconds traffic_stats::instantiated_ago( traffic_stats::time_point tpNow ) const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    return std::chrono::duration_cast< nanoseconds >( tpNow - instantiated() );
}
traffic_stats::nanoseconds traffic_stats::instantiated_ago() const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    return instantiated_ago( clock::now() );
}
traffic_stats::time_point traffic_stats::changed() const {
    lock_type lock( const_cast< myrt >( *this ) );
    switch( elisctt_ ) {
    case elisctt_instantiated:
        return time_stamp_instantiated_;
    case elisctt_opened:
        return time_stamp_opened_;
    case elisctt_closed:
        return time_stamp_closed_;
    default:
        return clock::now();
    }
}
traffic_stats::nanoseconds traffic_stats::changed_ago( traffic_stats::time_point tpNow ) const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    return std::chrono::duration_cast< nanoseconds >( tpNow - changed() );
}
traffic_stats::nanoseconds traffic_stats::changed_ago() const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    return changed_ago( clock::now() );
}

void traffic_stats::log_open() {
    lock_type lock( *this );
    elisctt_ = elisctt_opened;
    time_stamp_opened_ = clock::now();
}
void traffic_stats::log_close() {
    lock_type lock( *this );
    elisctt_ = elisctt_closed;
    time_stamp_closed_ = clock::now();
}

size_t traffic_stats::g_nDefaultEventQueueSizeForWebSocket = 10;
const char traffic_stats::g_strEventNameWebSocketFail[] = "fail";
const char traffic_stats::g_strEventNameWebSocketMessagesRecvText[] = "rx-txt";
const char traffic_stats::g_strEventNameWebSocketMessagesRecvBinary[] = "rx-bin";
const char traffic_stats::g_strEventNameWebSocketMessagesRecv[] = "rx";
const char traffic_stats::g_strEventNameWebSocketMessagesSentText[] = "tx-txt";
const char traffic_stats::g_strEventNameWebSocketMessagesSentBinary[] = "tx-bin";
const char traffic_stats::g_strEventNameWebSocketMessagesSent[] = "tx";
void traffic_stats::registrer_default_event_queues_for_web_socket() {
    event_queue_add( g_strEventNameWebSocketFail, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesRecvText, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesRecvBinary, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesRecv, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesSentText, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesSentBinary, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketMessagesSent, g_nDefaultEventQueueSizeForWebSocket );
}
const char traffic_stats::g_strEventNameWebSocketPeerConnect[] = "peer connect";
const char traffic_stats::g_strEventNameWebSocketPeerDisconnect[] = "peer disconnect";
const char traffic_stats::g_strEventNameWebSocketPeerDisconnectFail[] = "peer disconnect fail";
void traffic_stats::registrer_default_event_queues_for_web_socket_peer() {
    registrer_default_event_queues_for_web_socket();
    event_queue_add( g_strEventNameWebSocketPeerConnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketPeerDisconnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketPeerDisconnectFail, g_nDefaultEventQueueSizeForWebSocket );
}
const char traffic_stats::g_strEventNameWebSocketServerStart[] = "server start";
const char traffic_stats::g_strEventNameWebSocketServerStartFail[] = "server start fail";
const char traffic_stats::g_strEventNameWebSocketServerStop[] = "server stop";
void traffic_stats::registrer_default_event_queues_for_web_socket_server() {
    registrer_default_event_queues_for_web_socket();
    event_queue_add( g_strEventNameWebSocketPeerConnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketPeerDisconnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketPeerDisconnectFail, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketServerStart, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketServerStartFail, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketServerStop, g_nDefaultEventQueueSizeForWebSocket );
}
const char traffic_stats::g_strEventNameWebSocketClientConnect[] = "connect";
const char traffic_stats::g_strEventNameWebSocketClientConnectFail[] = "fail connect";
const char traffic_stats::g_strEventNameWebSocketClientDisconnect[] = "disconnect";
const char traffic_stats::g_strEventNameWebSocketClientReconnect[] = "reconnect attempt";
void traffic_stats::registrer_default_event_queues_for_web_socket_client() {
    registrer_default_event_queues_for_web_socket();
    event_queue_add( g_strEventNameWebSocketClientConnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketClientConnectFail, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketClientDisconnect, g_nDefaultEventQueueSizeForWebSocket );
    event_queue_add( g_strEventNameWebSocketClientReconnect, g_nDefaultEventQueueSizeForWebSocket );
}

bool traffic_stats::empty() const {
    lock_type lock( const_cast< myrt >( *this ) );
    if( text_tx_ != 0 || text_rx_ != 0 || bin_tx_ != 0 || bin_rx_ != 0 )
        return false;
    return true;
}
void traffic_stats::clear() {
    lock_type lock( *this );
    text_tx_ = text_rx_ = bin_tx_ = bin_rx_ = 0;
    elisctt_ = elisctt_instantiated;
    time_stamp_instantiated_ = time_stamp_opened_ = time_stamp_closed_ = clock::now();
    traffic_queue_all_tx_.clear();
    traffic_queue_all_rx_.clear();
    traffic_queue_text_tx_.clear();
    traffic_queue_text_rx_.clear();
    traffic_queue_bin_tx_.clear();
    traffic_queue_bin_rx_.clear();
    named_event_stats::clear();
}
void traffic_stats::init() {
    lock_type lock( *this );
    clear();
    limit( 50 );
}
traffic_stats::myrt traffic_stats::limit( size_t lim ) {
    traffic_queue_all_tx_.limit( lim );
    traffic_queue_all_rx_.limit( lim );
    traffic_queue_text_tx_.limit( lim );
    traffic_queue_text_rx_.limit( lim );
    traffic_queue_bin_tx_.limit( lim );
    traffic_queue_bin_rx_.limit( lim );
    return ( *this );
}
int traffic_stats::compare( traffic_stats::myrct x ) const {
    lock_type lock1( const_cast< myrt >( *this ) );
    lock_type lock2( const_cast< myrt >( x ) );
    if( int( elisctt_ ) < int( x.elisctt_ ) )
        return -1;
    if( int( elisctt_ ) > int( x.elisctt_ ) )
        return 1;
    // time_stamp_instantiated_, time_stamp_opened_, time_stamp_closed_ ... not used
    if( bytes_count_t( text_tx_ ) < bytes_count_t( x.text_tx_ ) )
        return -1;
    if( bytes_count_t( text_tx_ ) > bytes_count_t( x.text_tx_ ) )
        return 1;
    if( bytes_count_t( text_rx_ ) < bytes_count_t( x.text_rx_ ) )
        return -1;
    if( bytes_count_t( text_rx_ ) > bytes_count_t( x.text_rx_ ) )
        return 1;
    if( bytes_count_t( bin_tx_ ) < bytes_count_t( x.bin_tx_ ) )
        return -1;
    if( bytes_count_t( bin_tx_ ) > bytes_count_t( x.bin_tx_ ) )
        return 1;
    if( bytes_count_t( bin_rx_ ) < bytes_count_t( x.bin_rx_ ) )
        return -1;
    if( bytes_count_t( bin_rx_ ) > bytes_count_t( x.bin_rx_ ) )
        return 1;
    int n;
    n = traffic_queue_all_tx_.compare( x.traffic_queue_all_tx_ );
    if( n )
        return n;
    n = traffic_queue_all_rx_.compare( x.traffic_queue_all_rx_ );
    if( n )
        return n;
    n = traffic_queue_text_tx_.compare( x.traffic_queue_text_tx_ );
    if( n )
        return n;
    n = traffic_queue_text_rx_.compare( x.traffic_queue_text_rx_ );
    if( n )
        return n;
    n = traffic_queue_bin_tx_.compare( x.traffic_queue_bin_tx_ );
    if( n )
        return n;
    n = traffic_queue_bin_rx_.compare( x.traffic_queue_bin_rx_ );
    if( n )
        return n;
    n = named_event_stats::compare( x );
    if( n )
        return n;
    return 0;
}
traffic_stats::myrt traffic_stats::assign( traffic_stats::myrct x ) {
    lock_type lock1( *this );
    lock_type lock2( const_cast< myrt >( x ) );
    elisctt_ = x.elisctt_;
    time_stamp_instantiated_ = x.time_stamp_instantiated_;
    time_stamp_opened_ = x.time_stamp_opened_;
    time_stamp_closed_ = x.time_stamp_closed_;
    text_tx_ = bytes_count_t( x.text_tx_ );
    text_rx_ = bytes_count_t( x.text_rx_ );
    bin_tx_ = bytes_count_t( x.bin_tx_ );
    bin_rx_ = bytes_count_t( x.bin_rx_ );
    traffic_queue_all_tx_ = x.traffic_queue_all_tx_;
    traffic_queue_all_rx_ = x.traffic_queue_all_rx_;
    traffic_queue_text_tx_ = x.traffic_queue_text_tx_;
    traffic_queue_text_rx_ = x.traffic_queue_text_rx_;
    traffic_queue_bin_tx_ = x.traffic_queue_bin_tx_;
    traffic_queue_bin_rx_ = x.traffic_queue_bin_rx_;
    named_event_stats::assign( x );
    return ( *this );
}
traffic_stats::myrt traffic_stats::move( traffic_stats::myrt x ) {
    lock_type lock1( *this );
    lock_type lock2( x );
    assign( x );
    x.clear();
    return ( *this );
}

void traffic_stats::lock() {}
void traffic_stats::unlock() {}

std::string traffic_stats::getLifeTimeDescription( traffic_stats::time_point tpNow, bool isColored /*= false*/ ) const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    std::string strChangeType( last_instance_state_changing_type_as_str() ), strAgoSuffix( " ago" ), strSpace( " " );
    if( isColored ) {
        strChangeType = cc::debug( strChangeType );
        strAgoSuffix = cc::debug( strAgoSuffix );
        strSpace = cc::debug( strSpace );
    }
    nanoseconds nsx = changed_ago( tpNow );
    uint64_t ns = nsx.count();
    std::stringstream ss;
    ss << strChangeType << strSpace << tiny::tools::nanoseconds_2_lifetime_str( ns, isColored ) << strAgoSuffix;
    return ss.str();
}
std::string traffic_stats::getLifeTimeDescription( bool isColored /*= false*/ ) const {
    return getLifeTimeDescription( clock::now(), isColored );
}
std::string traffic_stats::getTrafficStatsDescription( traffic_stats::time_point tpNow,
                                                       bool isColored /*= false*/ ) const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    traffic_stats copy_of_this( *this );  // lock-free copy of this
    std::stringstream ss;
    bytes_count_t nTx = bytes_count_t( copy_of_this.tx() ), nRx = bytes_count_t( copy_of_this.rx() );
    double lfTxBPS = copy_of_this.bps_tx( tpNow ), lfRxBPS = copy_of_this.bps_rx( tpNow );
    std::string strBytesPrefix( "traffic " ), strBytesSuffix( " byte(s)" ), strTx( "Tx" ), strRx( "Rx" ),
        strTxValue( tiny::tools::to_string( nTx ) ), strRxValue( tiny::tools::to_string( nRx ) ),
        strBpsPrefix( " at " ), strBpsSuffix( " bps" ), strTxBPS, strRxBPS, strBpsComposed, strSlash( "/" ),
        strSpace( " " );
    bool bHaveBpsInfo = ( lfTxBPS > 0.0 || lfRxBPS > 0.0 ) ? true : false;
    if( bHaveBpsInfo ) {
        strTxBPS = tiny::tools::format( "%.03lf", lfTxBPS );
        strRxBPS = tiny::tools::format( "%.03lf", lfRxBPS );
    }
    else {
        strBpsComposed = "N/A";
    }
    if( isColored ) {
        strBytesPrefix = cc::debug( strBytesPrefix );
        strBytesSuffix = cc::debug( strBytesSuffix );
        strTx = cc::ws_tx( strTx );
        strRx = cc::ws_rx( strRx );
        strTxValue = cc::ws_tx( strTxValue );
        strRxValue = cc::ws_rx( strRxValue );
        if( !strTxBPS.empty() )
            strTxBPS = cc::ws_tx( strTxBPS );
        if( !strRxBPS.empty() )
            strRxBPS = cc::ws_rx( strRxBPS );
        if( !strBpsComposed.empty() )
            strBpsComposed = cc::error( strBpsComposed );
        strSlash = cc::debug( strSlash );
        strSpace = cc::debug( strSpace );
        if( !strBpsPrefix.empty() )
            strBpsPrefix = cc::debug( strBpsPrefix );
        if( !strBpsSuffix.empty() )
            strBpsSuffix = cc::debug( strBpsSuffix );
    }
    if( bHaveBpsInfo )
        strBpsComposed = strTxBPS + strSlash + strRxBPS;
    ss << strBytesPrefix << strTx << strSlash << strRx << strSpace << strTxValue << strSlash << strRxValue
       << strBytesSuffix << strBpsPrefix << strBpsComposed << strBpsSuffix;
    return ss.str();
}
std::string traffic_stats::getTrafficStatsDescription( bool isColored /*= false*/ ) const {
    return getTrafficStatsDescription( clock::now(), isColored );
}

nlohmann::json traffic_stats::toJSON( time_point tpNow, bool bSkipEmptyStats /*= true*/ ) const {
    // lock_type lock( const_cast < myrt > ( *this ) );
    nlohmann::json jo = tiny::stats::named_event_stats::toJSON( tpNow, bSkipEmptyStats, "events" );
    traffic_stats copy_of_this( *this );  // lock-free copy of this
    double lfTxBPS = copy_of_this.bps_tx( tpNow );
    double lfRxBPS = copy_of_this.bps_rx( tpNow );
    bytes_count_t nTx = bytes_count_t( copy_of_this.tx() );
    bytes_count_t nRx = bytes_count_t( copy_of_this.rx() );
    jo[ "life_time" ] =
        tiny::tools::nanoseconds_2_lifetime_str( copy_of_this.instantiated_ago( tpNow ).count(), false );
    jo[ "state" ][ "name" ] = copy_of_this.last_instance_state_changing_type_as_str();
    jo[ "state" ][ "time_stamp" ] =
        tiny::tools::nanoseconds_2_lifetime_str( copy_of_this.changed_ago( tpNow ).count(), false );
    jo[ "traffic" ][ "tx" ] = nTx;
    jo[ "traffic" ][ "rx" ] = nRx;
    jo[ "bps" ][ "tx" ] = lfTxBPS;
    jo[ "bps" ][ "rx" ] = lfRxBPS;
    return jo;
}
nlohmann::json traffic_stats::toJSON( bool bSkipEmptyStats /*= false*/ ) const {
    return toJSON( clock::now(), bSkipEmptyStats );
}

guarded_traffic_stats::guarded_traffic_stats()
//: traffic_stats_mtx_( "RMTX-TRAFFIC-STATS" )
{}
guarded_traffic_stats::guarded_traffic_stats( traffic_stats::myrct x )
: traffic_stats( x )
//, traffic_stats_mtx_( "RMTX-TRAFFIC-STATS" )
{}
guarded_traffic_stats::guarded_traffic_stats( traffic_stats::myrrt x )
: traffic_stats( x )
//, traffic_stats_mtx_( "RMTX-TRAFFIC-STATS" )
{}
guarded_traffic_stats::~guarded_traffic_stats() {}
void guarded_traffic_stats::lock() {
    // traffic_stats_mtx_.lock();
    tiny::get_ref_mtx().lock();
}
void guarded_traffic_stats::unlock() {
    // traffic_stats_mtx_.unlock();
    tiny::get_ref_mtx().unlock();
}
