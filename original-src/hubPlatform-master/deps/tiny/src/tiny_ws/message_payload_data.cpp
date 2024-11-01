message_payload_data::message_payload_data()
: pBuffer_( nullptr )
, cnt_( 0 )
, type_( LWS_WRITE_TEXT )
, isContinuation_( false ) {}
message_payload_data::message_payload_data( const message_payload_data& other )
: pBuffer_( nullptr )
, cnt_( 0 )
, type_( LWS_WRITE_TEXT )
, isContinuation_( false ) {
    assign( other );
}
message_payload_data::message_payload_data( message_payload_data&& other )
: pBuffer_( nullptr )
, cnt_( 0 )
, type_( LWS_WRITE_TEXT )
, isContinuation_( false ) {
    move( other );
}
message_payload_data& message_payload_data::operator=( const message_payload_data& other ) {
    assign( other );
    return ( *this );
}
message_payload_data& message_payload_data::operator=( message_payload_data&& other ) {
    move( other );
    return ( *this );
}
message_payload_data::~message_payload_data() {
    clear();
}
bool message_payload_data::empty() const {
    return ( size() == 0 ) ? true : false;
}
void message_payload_data::clear() {
    free();
    type_ = LWS_WRITE_TEXT;
}
void message_payload_data::fetchHeadPartAndMarkAsContinuation( size_t cntToFetch ) {
    if( cntToFetch == 0 )
        return;
    if( cnt_ == 0 )
        return;  // ???
    if( cntToFetch > cnt_ )
        cntToFetch = cnt_;
    isContinuation_ = true;
    if( cntToFetch == cnt_ )  // we will not realloc here for speed issues, just decrement cnt_
    {
        cnt_ = 0;
        return;
    }
    uint8_t* p = data();
    if( !p )
        return;
    cnt_ -= cntToFetch;
    if( cnt_ > 0 )
        ::memmove( p, p + cntToFetch, cnt_ );
}
size_t message_payload_data::pre() {
    return LWS_SEND_BUFFER_PRE_PADDING;
}
size_t message_payload_data::post() {
    return LWS_SEND_BUFFER_POST_PADDING;
}
void message_payload_data::free() {
    if( pBuffer_ ) {
        ::free( pBuffer_ );
        pBuffer_ = nullptr;
    }
    cnt_ = 0;
}
uint8_t* message_payload_data::alloc( size_t cnt ) {
    clear();
    uint8_t* p = (uint8_t*) ::malloc( pre() + cnt + post() );
    if( p ) {
        pBuffer_ = p;
        cnt_ = cnt;
    }
    return data();
}
uint8_t* message_payload_data::realloc( size_t cnt ) {
    if( !pBuffer_ )
        return alloc( cnt );
    uint8_t* p_new = (uint8_t*) ::realloc( pBuffer_, pre() + cnt + post() );
    if( !p_new )
        return nullptr;
    pBuffer_ = p_new;
    cnt_ = cnt;
    return data();
}
void message_payload_data::assign( const message_payload_data& other ) {
    if( this == ( &other ) )
        return;
    clear();
    size_t cnt = other.size();
    if( cnt == 0 )
        return;
    uint8_t* p = alloc( cnt );
    if( !p )
        return;
    ::memcpy( p, other.data(), cnt );
    type_ = other.type_;
    isContinuation_ = other.isContinuation_;
}
void message_payload_data::move( message_payload_data& other ) {
    if( this == ( &other ) )
        return;
    clear();
    pBuffer_ = other.pBuffer_;
    cnt_ = other.cnt_;
    type_ = other.type_;
    isContinuation_ = other.isContinuation_;
    other.pBuffer_ = nullptr;
    other.cnt_ = 0;
    other.isContinuation_ = false;
}
lws_write_protocol message_payload_data::type() const {
    return type_;
}
size_t message_payload_data::size() const {
    return cnt_;
}
const uint8_t* message_payload_data::data() const {
    return pBuffer_ + pre();
}
uint8_t* message_payload_data::data() {
    return pBuffer_ + pre();
}
uint8_t* message_payload_data::set_text( const char* s, size_t cnt ) {
    clear();
    size_t cntEffective = ( s == nullptr || cnt == 0 ) ? 0 : cnt;
    uint8_t* p = alloc( cntEffective );
    type_ = LWS_WRITE_TEXT;
    if( p && cntEffective > 0 )
        ::memcpy( p, s, cntEffective );
    return p;
}
uint8_t* message_payload_data::set_text( const char* s ) {
    size_t cnt = s ? ::strlen( s ) : 0;
    return set_text( s, cnt );
}
uint8_t* message_payload_data::set_text() {
    return set_text( nullptr, 0 );
}
uint8_t* message_payload_data::set_text( const std::string& s ) {
    return s.empty() ? set_text( nullptr, 0 ) : set_text( s.c_str(), s.length() );
}
uint8_t* message_payload_data::set_binary( const uint8_t* pBinary, size_t cnt ) {
    clear();
    size_t cntEffective = ( pBinary == nullptr || cnt == 0 ) ? 0 : cnt;
    uint8_t* p = alloc( cntEffective );
    type_ = LWS_WRITE_BINARY;
    if( p && cntEffective > 0 )
        ::memcpy( p, pBinary, cntEffective );
    return p;
}
uint8_t* message_payload_data::set_binary( const std::string& s ) {
    return s.empty() ? set_binary( nullptr, 0 ) : set_binary( (uint8_t*) &s.front(), s.size() );
}
uint8_t* message_payload_data::set_binary() {
    return set_binary( nullptr, 0 );
}
void message_payload_data::store_to_string( std::string& s ) const {
    s.clear();
    if( empty() )
        return;
    const uint8_t* p = data();
    size_t cntHave = size();
    if( type_ == LWS_WRITE_TEXT ) {
        if( p && cntHave > 0 )
            s.insert( 0, (char*) p, cntHave );
        return;
    }
    s = std::string( p, p + cnt_ );
}
bool message_payload_data::send_to_wsi( struct lws* wsi,
                                        size_t cntMaxPortionBytesToSend,
                                        size_t& cntLeft,
                                        std::string* pStrErrorDescription /*= nullptr*/ ) const {
    if( pStrErrorDescription )
        pStrErrorDescription->clear();
    cntLeft = 0;
    if( wsi == nullptr ) {
        if( pStrErrorDescription )
            ( *pStrErrorDescription ) = "wsi is NULL";
        return false;
    }
    size_t cnt = size();
    if( cnt <= 0 ) {
        if( pStrErrorDescription )
            ( *pStrErrorDescription ) = "no data to send";
        return false;
    }
    lws_write_protocol t = LWS_WRITE_CONTINUATION, tt = type();
    uint8_t* p = (uint8_t*) data();
    size_t cntPortion = cnt;
    bool isCtnn = isContinuation();
    bool isOverPortion = ( cntMaxPortionBytesToSend > 0 && cntPortion > cntMaxPortionBytesToSend ) ? true : false;
    if( isCtnn || isOverPortion ) {
        // for more details see
        // https://stackoverflow.com/questions/33916549/libwebsocket-send-big-messages-with-limited-payload
        if( isOverPortion )
            cntPortion = cntMaxPortionBytesToSend;
        cntLeft = cnt - cntPortion;
        if( !isCtnn )
            t = tt;
        if( cntLeft > 0 )
            t = lws_write_protocol( int( t ) | int( LWS_WRITE_NO_FIN ) );
    }
    else
        t = tt;
    if( tiny::ws::g_eWSLL == tiny::ws::e_ws_logging_level_t::eWSLL_detailed ) {
        std::cerr << "X>>>|cntPortion = " << cntPortion << "\n";
        std::string xxx;
        store_to_string( xxx );
        std::cerr << "B>>>|" << xxx << "|\n";
    }
    int cntSent = lws_write( wsi, p, cntPortion, t );
    if( tiny::ws::g_eWSLL == tiny::ws::e_ws_logging_level_t::eWSLL_detailed ) {
        std::cerr << "X>>>|cntSent = " << cntSent << "\n";
    }
    if( cntSent < 0 ) {
        if( pStrErrorDescription )
            ( *pStrErrorDescription ) = tiny::tools::format( "data(%d byte%s) was not sent(result is %d=0x%X)",
                                                             cntPortion,
                                                             ( cntPortion > 1 ) ? "s" : "",
                                                             cntSent,
                                                             cntSent );
        return false;
    }
    ( const_cast< message_payload_data* >( this ) )->fetchHeadPartAndMarkAsContinuation( cntSent );
    if( tiny::ws::g_eWSLL == tiny::ws::e_ws_logging_level_t::eWSLL_detailed ) {
        std::string xxx;
        store_to_string( xxx );
        std::cerr << "A>>>|" << xxx << "|\n";
    }
    // if( cntSent >= cnt )
    //	return true;
    cntLeft = size();
    if( isOverPortion ) {
        // Have more data to send
        ::lws_callback_on_writable( wsi );
    }
    return true;
}
bool message_payload_data::append( const message_payload_data& other ) {
    size_t cntBefore = size();
    lws_write_protocol t = type(), t_other = other.type();
    if( t != t_other && cntBefore > 0 )
        return false;
    type_ = t_other;  // auto change type when assigning to this empty data object
    size_t cntOther = other.size();
    if( cntOther == 0 )
        return true;
    if( cntBefore == 0 ) {
        assign( other );
        return true;
    }
    size_t cntAfter = cntBefore + cntOther;
    uint8_t* p = realloc( cntAfter );
    if( !p )
        return false;
    ::memcpy( p + cntBefore, other.data(), cntOther );
    return true;
}
