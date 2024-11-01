client_api::client_api() {
    clear_fields();
}
client_api::~client_api() {
    deinit();
}

void client_api::clear_fields() {
    onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "WS Client: clear all fields" );

    lock_type lock( mtx_api() );
    delayed_de_init_ = false;
    basic_api::clear_fields();
    cid_ = 0;
    vec_lws_protocols_[ default_protocol_index_ ].callback = stat_callback_client;
    destroy_flag_ = false;
    connection_flag_ = false;
    // writeable_flag_ = false;
    //
    strURL_.clear();
    wsi_ = nullptr;
    ssl_flags_ = 0;
    cert_path_.clear();
    key_path_.clear();
    ca_path_.clear();
    buffer_.clear();
    //
    accumulator_.clear();
}
bool client_api::init( const std::string& strURL, security_args* pSA ) {
    lock_type lock( mtx_api() );
    tiny::url an_url( strURL );
    const auto& scheme = an_url.scheme();
    bool isSSL = ( scheme == "ws" ) ? false : true;
    std::string strHost = an_url.host();
    std::string strPath = an_url.path();
    int nPort = ::atoi( an_url.port().c_str() );
    return init( isSSL, strHost, nPort, strPath, pSA );
}

extern "C" void lws_ssl_elaborate_error();
extern "C" void lws_ssl_bind_passphrase( SSL_CTX* ssl_ctx, struct lws_context_creation_info* info );

bool client_api::init( bool isSSL,
                       const std::string& strHost,
                       int nPort,
                       const std::string& strPath,
                       security_args* pSA ) {
    if( initialized_ )
        return false;

    onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "WS Client: start initializing connection..." );

    lock_type lock( mtx_api() );

    deinit();

    bool bDoInitSSL = false;
    if( isSSL && pSA != nullptr ) {
        if( ssl_perform_global_init_ )
            bDoInitSSL = true;
        cert_path_ = pSA->strCertificateFile_;
        key_path_ = pSA->strPrivateKeyFile_;
        ca_path_ = pSA->strCertificationChainFile_;

        ssl_flags_ |= LCCSCF_USE_SSL;

        if( ssl_client_allow_self_signed_ )
            ssl_flags_ |= LCCSCF_ALLOW_SELFSIGNED;

        if( ssl_client_skip_host_name_check_ )
            ssl_flags_ |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

        if( ssl_client_allow_expired_ )
            ssl_flags_ |= LCCSCF_ALLOW_EXPIRED;

        ssl_flags_ |= LCCSCF_PIPELINE;
        //
        //
        ctx_info_.client_ssl_cert_filepath = ( !cert_path_.empty() ) ? cert_path_.c_str() : nullptr;
        ctx_info_.client_ssl_private_key_filepath = ( !key_path_.empty() ) ? key_path_.c_str() : nullptr;
        ctx_info_.client_ssl_ca_filepath = ( !ca_path_.empty() ) ? ca_path_.c_str() : nullptr;

        //
        //
    }  // i( isSSL && pSA != nullptr )

    if( bDoInitSSL )  // ssl_perform_global_init_
        ctx_info_.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    ctx_info_.options |= LWS_SERVER_OPTION_JUST_USE_RAW_ORIGIN;

    ctx_info_.ws_ping_pong_interval = interval_ping_;

    //				ctx_info_.ssl_options_set = // compatibility with old implementation based on wspp
    //						SSL_OP_NO_SSLv2 //| SSL_OP_NO_SSLv3
    //						| SSL_OP_SINGLE_DH_USE
    //						//| SSL_OP_ALL
    //						;

    vec_lws_protocols_[ default_protocol_index_ ].per_session_data_size = 0;  // sizeof(struct client_session_data);

    const size_t client_api_init_rx_buffer_size =
        1024 *
        100;  // rx_buffer_size, lws allocates this much space for rx data and informs callback when something came
    size_t rx_buffer_size_tuned = g_nDefaultClientBufferSizeRX > client_api_init_rx_buffer_size ?
                                      g_nDefaultClientBufferSizeRX :
                                      client_api_init_rx_buffer_size;

    vec_lws_protocols_[ default_protocol_index_ ].rx_buffer_size = rx_buffer_size_tuned;

    vec_lws_protocols_[ default_protocol_index_ ].user =
        this;  // user, by lws, but user code can pass a pointer here it can later access from the protocol callback
    vec_lws_protocols_[ default_protocol_index_ ].tx_packet_size =
        0;  // tx_packet_size, 0 indicates restrict send() size to .rx_buffer_size for backwards-compatibility

    ctx_info_.client_tls_1_3_plus_cipher_list = nullptr;
    ctx_info_.client_ssl_cipher_list = nullptr
        //					"ECDHE-ECDSA-AES256-GCM-SHA384:"
        //					"ECDHE-RSA-AES256-GCM-SHA384:"
        //					"DHE-RSA-AES256-GCM-SHA384:"
        //					"ECDHE-RSA-AES256-SHA384:"
        //					"HIGH:!aNULL:!eNULL:!EXPORT:"
        //					"!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
        //					"!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
        //					"!DHE-RSA-AES128-SHA256:"
        //					"!AES128-GCM-SHA256:"
        //					"!AES128-SHA256:"
        //					"!DHE-RSA-AES256-SHA256:"
        //					"!AES256-GCM-SHA384:"
        //					"!AES256-SHA256"
        ;
    /*
    ctx_info_.simultaneous_ssl_restriction = 4; // ???
    ctx_info_.client_ssl_ca_filepath = "/opt/nma/certs/server.ca";

    LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION
    LWS_CALLBACK_SSL_INFO ssl_info_event_mask

            LWS_SERVER_OPTION_SSL_ECDH
            LWS_SERVER_OPTION_CREATE_VHOST_SSL_CTX
    */

    ctx_ = ::lws_create_context( &ctx_info_ );
    if( ctx_ == nullptr ) {
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error, "NLWS: Failed to create client context" );
        return false;
    }
    std::string sp = ( !strPath.empty() ) ? strPath.c_str() : "/";
    strURL_ = tiny::tools::format( "%s://%s:%d%s", ssl_flags_ ? "wss" : "ws", strHost.c_str(), nPort, sp.c_str() );
    std::string strOrigin = tiny::tools::format( "%s:%d", strHost.c_str(), nPort );
    //				wsi_ =
    //					::lws_client_connect(
    //						ctx_,
    //						strHost.c_str(),
    //						nPort,
    //						ssl_flags_,
    //						sp.c_str(),
    //						strOrigin.c_str(), // "origin"
    //						nullptr,
    //						g_strDefaultProtocolName.c_str(), // protocols_[cpi].name
    //						-1
    //						);
    struct lws_client_connect_info cci;
    ::memset( &cci, 0, sizeof( struct lws_client_connect_info ) );
    cci.context = ctx_;
    cci.ssl_connection = ssl_flags_;
    cci.address = strHost.c_str();
    cci.port = nPort;
    cci.path = sp.c_str();
    cci.host = strHost.c_str();
    cci.origin = strHost.c_str();  // "origin"
    cci.ietf_version_or_minus_one = -1;
    cci.pwsi = &wsi_;
    wsi_ = ::lws_client_connect_via_info( &cci );
    if( wsi_ == nullptr ) {
        deinit();
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error, "NLWS: client wsi creation fail" );
        return false;
    }

    configure_wsi( wsi_ );
    stat_reg( this );

    clientThreadStopFlag_ = false;

    clientThread_ = std::thread( [this]() {
        onLogMessage( e_ws_log_message_type_t::eWSLMT_info,
                      cc::info( "NLWS: Client connection polling thread " ) + cc::note( "started" ) );
        int result = 0;
        while( !clientThreadStopFlag_ && ctx_ ) {
            result = ::lws_service( ctx_, 50 );
            if( result < 0 ) {
                onLogMessage( e_ws_log_message_type_t::eWSLMT_error,
                              cc::info( "NLWS: service returns: " ) + cc::num10( result ) );
            }
        }
        onLogMessage( e_ws_log_message_type_t::eWSLMT_info,
                      cc::info( "NLWS: Client connection polling thread " ) + cc::note( "closed" ) );
    } );

    // do_writable_callbacks_all_protocol();
    // while( ! ( connection_flag_ || destroy_flag_ ) )
    //	std::this_thread::sleep_for( std::chrono::milliseconds(20) );
    initialized_ = true;
    return true;
}
void client_api::deinit() {
    onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "WS Client: deinit connection structures." );
    clientThreadStopFlag_ = true;
    try {
        lock_type lock( clientThreadMutex_ );
        if( clientThread_.joinable() ) {
            if( ctx_ ) {
                ::lws_cancel_service( ctx_.load() );
            }

            clientThread_.join();
        }
    }
    catch( ... ) {
    }
    lock_type lock( mtx_api() );
    // wsi_ ???
    if( ctx_ ) {
        auto ctx2destroy = ctx_.load();
        ctx_ = nullptr;
        ::lws_context_destroy( ctx2destroy );
    }

    stat_unreg( this );

    clear_fields();
    delayed_close_reason_.clear();
    delayed_close_status_ = 0;
}
void client_api::close( int nCloseStatus, const std::string& msg ) {
    lock_type lock( mtx_api() );
    if( !initialized_ )
        return;
    if( wsi_ == nullptr )
        return;
    ///::lws_close_reason( wsi_, (lws_close_status)nCloseStatus, (unsigned char *)msg.c_str(), msg.length() );
    /// deinit();
    delayed_close_reason_ = msg;
    delayed_close_status_ = nCloseStatus;
}
void client_api::delay_deinit() {
    if( delayed_de_init_ )
        return;
    delayed_de_init_ = true;
    if( onDelayDeinit_ )
        onDelayDeinit_();
}

void client_api::onConnect() {
    if( onConnect_ )
        onConnect_();
}
void client_api::onDisconnect( const std::string& strMessage ) {
    if( onDisconnect_ )
        onDisconnect_( strMessage );
}
void client_api::onMessage( const message_payload_data& data, bool isFinalFragment ) {
    lock_type lock( mtx_api() );
    accumulator_.append( data );
    if( !isFinalFragment )
        return;
    message_payload_data final_data = std::move( accumulator_ );
    accumulator_.clear();
    if( onMessage_ )
        onMessage_( final_data );
}
void client_api::onFail( const std::string& strMessage ) {
    if( onFail_ )
        onFail_( strMessage );
}
void client_api::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& strMessage ) {
    if( onLogMessage_ )
        onLogMessage_( eWSLMT, strMessage );
}

bool client_api::send( const message_payload_data& data ) {
    if( !initialized_ )
        return false;
    {
        lock_type lock( mtx_api() );
        // Push this onto the buffer. It will be written out when the socket is writable.
        buffer_.push_back( data );
    }

    do_writable_callbacks_all_protocol();
    return true;
}

client_api::map_ctx_2_inst_t client_api::g_ctx_2_inst;
client_api::map_inst_2_ctx_t client_api::g_inst_2_ctx;
client_api::mutex_type client_api::g_ctx_reg_mtx;
void client_api::stat_reg( client_api* api ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( api );
    assert( api->wsi_ );
    g_ctx_2_inst[ api->wsi_ ] = api;
    g_inst_2_ctx[ api ] = api->wsi_;
}
void client_api::stat_unreg( client_api* api ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( api );
    if( !api->wsi_ )
        return;
    g_ctx_2_inst.erase( api->wsi_ );
    g_inst_2_ctx.erase( api );
}
client_api* client_api::stat_get( void* ctx ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( ctx );
    map_ctx_2_inst_t::iterator itFind = g_ctx_2_inst.find( ctx ), itEnd = g_ctx_2_inst.end();
    if( itFind == itEnd ) {
        //					assert( false );
        return nullptr;
    }
    client_api* api = itFind->second;
    //				assert( api );
    //				assert( api->wsi_ );
    //				assert( api->wsi_ == ctx );
    return api;
}
