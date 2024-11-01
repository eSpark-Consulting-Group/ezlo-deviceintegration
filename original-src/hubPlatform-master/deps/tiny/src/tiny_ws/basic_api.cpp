std::string basic_api::g_strDefaultProtocolName( "ws-home-automation" );  // "default-protocol"
// some experimental notes about max working connection counts with different buffer sizes on machine with 4GB RAM:
// 1024*128 - about 400   connections
// 1024*64  - about 800   connections
// 1024*32  - about 1600  connections
// 1024*16  - about 3200  connections
// 1024*8   - about 6400  connections
// 1024*4   - about 12800 connections
// 1024*2   - about 25600 connections
// 1024     - about 51200 connections
volatile size_t basic_api::g_nDefaultBufferSizeRX = 1024 * 16;  // 0 - unlimited
volatile size_t basic_api::g_nDefaultBufferSizeTX = 1024 * 16;  // 0 - same as g_nDefaultBufferSizeRX

// magic constant extracted from client_api, overwrites default protocol's rx_buffer_size
volatile size_t basic_api::g_nDefaultClientBufferSizeRX = 100 * 1024;

basic_api::basic_api()
//: mtx_api_( "RMTX-NLWS-BASIC-API" )
{
    clear_fields();
}
basic_api::~basic_api() {
    clear_fields();
}

basic_api::mutex_type& basic_api::mtx_api() const {
    // return mtx_api_;
    return tiny::get_ref_mtx();
}

void basic_api::locked_execute( fn_lock_callback_t fn ) {
    if( !fn )
        return;
    lock_type lock( mtx_api() );
    fn();
}

void basic_api::clear_fields() {
    bns_assign_from_default_instance();
    max_message_size_ = 0;  // unlimited
    max_body_size_ = 0;     // unlimited
    //
    switch( g_eWSLL ) {
    case e_ws_logging_level_t::eWSLL_basic:
        ::lws_set_log_level( LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, /*lwsl_emit_syslog*/ nullptr );
        break;
    case e_ws_logging_level_t::eWSLL_detailed:
        ::lws_set_log_level( 2047, /*lwsl_emit_syslog*/ nullptr );
        break;
    default:
        ::lws_set_log_level( 0, /*lwsl_emit_syslog*/ nullptr );
        break;
    }  // switch( g_eWSLL )
    //
    static const struct lws_extension g_exts[] = {
        { "permessage-deflate", lws_extension_callback_pm_deflate, "permessage-deflate" },
        { "deflate-frame", lws_extension_callback_pm_deflate, "deflate_frame" },
        { nullptr, nullptr, nullptr }  // terminator
    };
    //
    initialized_ = false;
    // while( cntTryLockExecutes_ > 0 )
    //	std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    ctx_ = nullptr;
    ::memset( &ctx_info_, 0, sizeof( struct lws_context_creation_info ) );
    ctx_info_.port = CONTEXT_PORT_NO_LISTEN;
    ctx_info_.iface = nullptr;
    // ctx_info_.protocols = vec_lws_protocols_.data(); // this MUST be done later

    ctx_info_.ssl_cert_filepath = nullptr;
    ctx_info_.ssl_private_key_filepath = nullptr;
    ctx_info_.ssl_ca_filepath = nullptr;
    //				ctx_info_.extensions = ::lws_get_internal_extensions();
    ctx_info_.extensions = g_exts;
    ctx_info_.gid = -1;
    ctx_info_.uid = -1;
    ctx_info_.options = 0;
    // keep alive
    ctx_info_.ka_time = 60;     // 60 seconds until connection is suspicious
    ctx_info_.ka_probes = 3;    // 3 probes after ^ time
    ctx_info_.ka_interval = 5;  // 5s interval for sending probes
    //
    ctx_info_.timeout_secs = 5;
    //
    unsigned int walk_protocol_id = 0;
    vec_lws_protocols_.clear();
    vec_lws_protocols_.reserve( 16 );  // try to avoid reallocation
    //
    //				vec_lws_protocols_.push_back( { // the first protocol must always be the HTTP handler
    //						"http-only",            // name
    //						stat_callback_http,     // callback
    //						0,                      // per_session_data_size, no per session data.
    //						g_nDefaultBufferSizeRX, // rx_buffer_size, max frame size / rx buffer
    //						walk_protocol_id,       // id, ignored by lws
    //						nullptr,                // user
    //						g_nDefaultBufferSizeTX  // tx_packet_size
    //					} );
    //				++ walk_protocol_id;
    //
    default_protocol_index_ = vec_lws_protocols_.size();
    vec_lws_protocols_.push_back( {
        g_strDefaultProtocolName.c_str(),
        stat_callback_server,
        0,                       // per_session_data_size
        g_nDefaultBufferSizeRX,  // rx_buffer_size, lws allocates this much space for rx data and informs callback when
        // something came
        walk_protocol_id,  // id, ignored by lws, but useful to contain user information bound to the selected protocol
        nullptr,  // user, by lws, but user code can pass a pointer here it can later access from the protocol callback
        g_nDefaultBufferSizeTX  // tx_packet_size, 0 indicates restrict send() size to .rx_buffer_size for
                                // backwards-compatibility
    } );
    ++walk_protocol_id;
    //

    vec_lws_protocols_.push_back( { nullptr, nullptr, 0, 0, 0, nullptr, 0 } );  // terminator
    ctx_info_.protocols = vec_lws_protocols_.data();  // this MUST be done here, can be modified in beforeCreateContext
    //
}

void basic_api::do_writable_callbacks_all_protocol() {
    ::lws_callback_on_writable_all_protocol( ctx_, &vec_lws_protocols_[ default_protocol_index_ ] );
}

static enum pending_timeout g_arrPingPongTimeoutTypes[]{
    PENDING_TIMEOUT_AWAITING_PING,
    PENDING_TIMEOUT_WS_PONG_CHECK_SEND_PING,
    PENDING_TIMEOUT_WS_PONG_CHECK_GET_PONG,
};
static enum pending_timeout g_arrOpenTimeoutTypes[]{
    PENDING_TIMEOUT_ESTABLISH_WITH_SERVER,
    PENDING_TIMEOUT_SENT_CLIENT_HANDSHAKE,
    PENDING_TIMEOUT_SSL_ACCEPT,
    PENDING_TIMEOUT_AWAITING_CLIENT_HS_SEND,
    PENDING_TIMEOUT_AWAITING_PROXY_RESPONSE,
    PENDING_TIMEOUT_AWAITING_CONNECT_RESPONSE,
    PENDING_TIMEOUT_AWAITING_SOCKS_GREETING_REPLY,
    PENDING_TIMEOUT_AWAITING_SOCKS_CONNECT_REPLY,
    PENDING_TIMEOUT_AWAITING_SOCKS_AUTH_REPLY,
};
static enum pending_timeout g_arrCloseTimeoutTypes[]{
    PENDING_TIMEOUT_SHUTDOWN_FLUSH,     PENDING_FLUSH_STORED_SEND_BEFORE_CLOSE,
    PENDING_TIMEOUT_KILLED_BY_SSL_INFO, PENDING_TIMEOUT_KILLED_BY_PARENT,
    PENDING_TIMEOUT_CLOSE_SEND,         PENDING_TIMEOUT_CLOSE_ACK,
};
static enum pending_timeout g_arrNetworkTimeoutTypes[]{
    PENDING_TIMEOUT_HTTP_CONTENT,
    PENDING_TIMEOUT_AWAITING_SERVER_RESPONSE,
    PENDING_TIMEOUT_AWAITING_CONNECT_RESPONSE,
    PENDING_TIMEOUT_HTTP_CONTENT,
    // PENDING_FLUSH_STORED_SEND_BEFORE_CLOSE,
    PENDING_TIMEOUT_HTTP_KEEPALIVE_IDLE,
    PENDING_TIMEOUT_CLIENT_ISSUE_PAYLOAD,
};
// PENDING_TIMEOUT_HOLDING_AH
void basic_api::configure_wsi( struct lws* wsi ) {
    if( wsi == nullptr )
        return;
    size_t i, cnt;
    if( timeout_handshake_open_ > 0 ) {
        for( i = 0, cnt = sizeof( g_arrOpenTimeoutTypes ) / sizeof( g_arrOpenTimeoutTypes[ 0 ] ); i < cnt; ++i )
            ::lws_set_timeout( wsi, g_arrOpenTimeoutTypes[ i ], timeout_handshake_open_ );
        for( i = 0, cnt = sizeof( g_arrNetworkTimeoutTypes ) / sizeof( g_arrNetworkTimeoutTypes[ 0 ] ); i < cnt; ++i )
            ::lws_set_timeout( wsi, g_arrNetworkTimeoutTypes[ i ], timeout_handshake_open_ );
    }
    if( timeout_handshake_close_ > 0 ) {
        for( i = 0, cnt = sizeof( g_arrCloseTimeoutTypes ) / sizeof( g_arrCloseTimeoutTypes[ 0 ] ); i < cnt; ++i )
            ::lws_set_timeout( wsi, g_arrCloseTimeoutTypes[ i ], timeout_handshake_close_ );
    }
    if( timeout_pong_ > 0 ) {
        for( i = 0, cnt = sizeof( g_arrPingPongTimeoutTypes ) / sizeof( g_arrPingPongTimeoutTypes[ 0 ] ); i < cnt;
             ++i ) {
            ::lws_set_timeout( wsi, g_arrPingPongTimeoutTypes[ i ], timeout_pong_ );
        }
    }
}

bool impl_frame_is_binary( struct lws* wsi, const uint8_t* in, size_t len ) {
    if(::lws_frame_is_binary( wsi ) )
        return true;
    return false;
}

int basic_api::stat_callback_http( struct lws* wsi,
                                   enum lws_callback_reasons reason,
                                   void* user,
                                   void* in,
                                   size_t len ) {
    bool isClose = false;
    switch( reason ) {
    case LWS_CALLBACK_ESTABLISHED:
        // ctx = ::lws_get_context( wsi );
        // self = server_api::stat_get( ctx );
        //::lws_callback_on_writable( wsi );
        isClose = true;
        break;
    case LWS_CALLBACK_HTTP:
        //::lws_serve_http_file( wsi, "example.html", "text/html", nullptr, 0 );
        isClose = true;
        break;
    default:
        break;
    }  // switch( reason )
    if( isClose ) {
        std::string strCloseReason( "HTTP is disabled" );
        ::lws_close_reason( wsi,
                            lws_close_status::LWS_CLOSE_STATUS_POLICY_VIOLATION,
                            (unsigned char*) strCloseReason.c_str(),
                            strCloseReason.length() );
        return -1;
    }
    return 0;
}

/// extern "C" void impl_lws_close_free_wsi( struct lws *wsi, enum lws_close_status reason, int bIsFree );

int basic_api::stat_callback_client( struct lws* wsi,
                                     enum lws_callback_reasons reason,
                                     void* user,
                                     void* in,
                                     size_t len ) {
    client_api* self = nullptr;
    switch( reason ) {
    // case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
    //	return 1; // disallow compression
    //				case LWS_CALLBACK_PROTOCOL_INIT: // per vhost
    //					lws_protocol_vh_priv_zalloc( lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(struct
    // per_vhost_data__lws_mirror) ); 				break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        self = client_api::stat_get( wsi );
        if( self ) {
            // self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "NLWS: LWS_CALLBACK_CLIENT_ESTABLISHED:
            // connect with server success" );
            self->cid_ = wsi;
            self->connection_flag_ = true;
            self->onConnect();
            ::lws_callback_on_writable( wsi );
        }
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        self = client_api::stat_get( wsi );
        if( self ) {
            // self->onLogMessage( e_ws_log_message_type_t::eWSLMT_error, "NLWS: LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            // connect with server error" );
            self->destroy_flag_ = true;
            self->connection_flag_ = false;
            self->clientThreadStopFlag_ = true;
            self->onFail( std::string( "NLWS: client connection error (via callback): " ) +
                          ( in == nullptr ? "<null>" : (char*) in ) );
        }
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
    case LWS_CALLBACK_CLOSED:
        self = client_api::stat_get( wsi );
        if( self ) {
            // self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "NLWS: LWS_CALLBACK_CLOSED" );
            self->destroy_flag_ = true;
            self->connection_flag_ = false;
            self->clientThreadStopFlag_ = true;
            self->onDisconnect( "NLWS: client connection closed" );
        }
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        self = client_api::stat_get( wsi );
        if( self ) {
            //						if( self->writeable_flag_ ) {
            //							//self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "NLWS:
            // LWS_CALLBACK_CLIENT_RECEIVE: client recvived data" ); 							self->destroy_flag_ =
            // true; self->onDisconnect( "read attempt when writable" ); 							return 0;
            //						}
            message_payload_data data;
            if( impl_frame_is_binary( wsi, (uint8_t*) in, len ) )
                data.set_binary( (uint8_t*) in, len );
            else
                data.set_text( (char*) in, len );
            ////size_t nRemain = ::lws_remaining_packet_payload( wsi ); bool isFinalFragment = ( nRemain == 0 ) ? true :
            /// false;
            bool isFinalFragment = ::lws_is_final_fragment( wsi );
            self->onMessage( data, isFinalFragment );
        }
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        self = client_api::stat_get( wsi );
        if( self ) {
            volatile bool bCloseAction = false;
            {  // block
                client_api::lock_type lock( self->mtx_api() );
                std::string cached_delayed_close_reason = self->delayed_close_reason_;
                lws_close_status cached_delayed_close_status = (lws_close_status) self->delayed_close_status_;
                self->delayed_close_reason_.clear();
                self->delayed_close_status_ = 0;
                //
                if( !cached_delayed_close_reason.empty() ) {
                    self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                                        cc::debug( "Processing close action for client connection" ) );
                    ::lws_close_reason( wsi,
                                        cached_delayed_close_status,
                                        (unsigned char*) cached_delayed_close_reason.c_str(),
                                        cached_delayed_close_reason.length() );
                    /// impl_lws_close_free_wsi( wsi, cached_delayed_close_status, 0 );
                    bCloseAction = true;
                    // self->onDisconnect( msg );
                    self->delay_deinit();
                }  // if( ! cached_delayed_close_reason.empty() )
                else {
                    payload_queue_t& pq = self->buffer_;
                    while( !pq.empty() ) {
                        const message_payload_data& data = pq.front();
                        size_t cntLeft = 0;
                        size_t cntMaxPortion = self->cntMaxPortionBytesToSend_;

                        if( g_nDefaultBufferSizeTX > 0 && cntMaxPortion < g_nDefaultBufferSizeTX )
                            cntMaxPortion = g_nDefaultBufferSizeTX;
                        std::string strErrorDescription;
                        if( !data.send_to_wsi( wsi, cntMaxPortion, cntLeft, &strErrorDescription ) ) {
                            std::string strFailMessage;
                            strFailMessage += "NLWS: error writing to socket(client writable callback)";
                            if( !strErrorDescription.empty() ) {
                                strFailMessage += ", error description: ";
                                strFailMessage += strErrorDescription;
                            }
                            self->onFail( strFailMessage );
                            break;
                        }
                        if( cntLeft > 0 )
                            break;
                        pq.pop_front();  // only pop the message if it was sent successfully
                    }                    // while( ! pq.empty() )
                    if( self->delayed_adjustment_pong_timeout_ >=
                        0 ) {  // -1 for no adjustment, otherwise change pong timeout
                        int64_t timeout_pong = self->delayed_adjustment_pong_timeout_;
                        self->delayed_adjustment_pong_timeout_ = -1;
                        size_t i, cnt;
                        for( i = 0,
                             cnt = sizeof( g_arrPingPongTimeoutTypes ) / sizeof( g_arrPingPongTimeoutTypes[ 0 ] );
                             i < cnt;
                             ++i ) {
                            ::lws_set_timeout( wsi, g_arrPingPongTimeoutTypes[ i ], timeout_pong );
                        }
                    }
                }  // else from if( ! cached_delayed_close_reason.empty() )
            }      // block
            if( bCloseAction )
                return -1;  // this closes connection accoriding to
            // https://libwebsockets.org/lws-api-doc-master/html/md_README_8coding.html
        }  // if( self )
        break;
        /*
                        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
                            lwsl_err( "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS\n" );
                        break;
        */

    case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION: {
        self = client_api::stat_get( wsi );
        if( !self )
            break;

        auto preverify_ok = len;
        if( !preverify_ok )
            break;

        auto ctx = static_cast< X509_STORE_CTX* >( user );

        auto cert = X509_STORE_CTX_get_current_cert( ctx );
        auto depth = X509_STORE_CTX_get_error_depth( ctx );
        // auto err = X509_STORE_CTX_get_error(ctx);
        // const char * msg = X509_verify_cert_error_string( err );
        // printf( "Depth: %d, SSL error: %s (%d), preverify_ok: %d\n", depth, msg, err, preverify_ok );

        if( depth != 0 )
            break;

        auto nm = X509_get_subject_name( cert );

        auto checkSubjectNameEntry = [self]( X509_NAME* name, int nid, const std::string& value ) -> bool {
            if( value.empty() ) {
                return true;
            }
            int pos = X509_NAME_get_index_by_NID( name, nid, -1 );
            if( pos == -1 ) {
                self->onLogMessage( e_ws_log_message_type_t::eWSLMT_error,
                                    cc::error( "Field " ) + cc::note( OBJ_nid2ln( nid ) ) +
                                        cc::error( " not found in certificate subject" ) );
                return false;
            }
            auto entry = X509_NAME_get_entry( name, pos );
            auto data = X509_NAME_ENTRY_get_data( entry );
            auto buff = ASN1_STRING_data( data );
            auto length = ASN1_STRING_length( data );
            bool ok = strncmp( reinterpret_cast< const char* >( buff ), value.c_str(), length ) == 0;

            if( !ok ) {
                self->onLogMessage( e_ws_log_message_type_t::eWSLMT_error,
                                    cc::error( "Certificate subject field " ) + cc::note( OBJ_nid2ln( nid ) ) +
                                        cc::error( " mismatch, expected " ) + cc::note( value ) +
                                        cc::error( ", got " ) + cc::note( reinterpret_cast< const char* >( buff ) ) );
            }
            return ok;
        };

        bool ok = true;
        ok &= checkSubjectNameEntry( nm, NID_organizationName, self->validOrganizationName_ );
        ok &= checkSubjectNameEntry( nm, NID_organizationalUnitName, self->validOrganizationalUnitName_ );
        ok &= checkSubjectNameEntry( nm, NID_commonName, self->validCommonName_ );
        if( !ok ) {
            X509_STORE_CTX_set_error( ctx, X509_V_ERR_APPLICATION_VERIFICATION );
            return 1;
        }
        break;
    }
    default: {
        if( g_eWSLL == e_ws_logging_level_t::eWSLL_detailed ) {
            self = client_api::stat_get( wsi );
            if( self ) {
                self->onLogMessage(
                    e_ws_log_message_type_t::eWSLMT_debug,
                    cc::debug( "stat_callback_client: recived: " ) + cc::note( std::to_string( reason ) ) );
            }
        }
    } break;

    }  // switch( reason )
    return 0;
}

int basic_api::stat_callback_server( struct lws* wsi,
                                     enum lws_callback_reasons reason,
                                     void* user,
                                     void* in,
                                     size_t len ) {
    lws_context* ctx = nullptr;
    server_api* self = nullptr;

    constexpr int MAX_CONNECTIONS_ALLOWED = 200;

    //				const size_t nPre = LWS_SEND_BUFFER_PRE_PADDING, nPost = LWS_SEND_BUFFER_POST_PADDING;
    //				unsigned char buf[ nPre + 512 + nPost ];
    //				unsigned char * p = &buf[nPre];

    switch( reason ) {
    // case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
    //	return 1; // disallow compression
    //				case LWS_CALLBACK_PROTOCOL_INIT: // per vhost
    //					lws_protocol_vh_priv_zalloc( lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(struct
    // per_vhost_data__lws_mirror) ); 				break;
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        // NOTE: Limit amount of local mode connections ( WSI_CREATE is called after FILTER_NETWORK_CONNECTION )
        if( self->overall_connections_count_ + 1 > MAX_CONNECTIONS_ALLOWED ) {
            return -1;
        }
        break;
    case LWS_CALLBACK_WSI_CREATE:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        self->overall_connections_count_++;
        break;
    case LWS_CALLBACK_WSI_DESTROY:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        self->overall_connections_count_--;
        break;
    case LWS_CALLBACK_TIMER:
        // NOTE: disconnect peer as it was not able to log in within specified timeout
        return -1;
    case LWS_CALLBACK_ESTABLISHED:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        if( self ) {
            self->configure_wsi( wsi );
            int fd = ::lws_get_socket_fd( wsi );
            char strPeerClientAddressName[ 256 ], strPeerRemoteIP[ 256 ];
            ::memset( strPeerClientAddressName, 0, sizeof( strPeerClientAddressName ) );
            ::memset( strPeerRemoteIP, 0, sizeof( strPeerRemoteIP ) );
            ::lws_get_peer_addresses( wsi,
                                      fd,
                                      strPeerClientAddressName,
                                      sizeof( strPeerClientAddressName ) - 1,
                                      strPeerRemoteIP,
                                      sizeof( strPeerRemoteIP ) - 1 );
            self->onConnect( wsi, wsi, strPeerClientAddressName, strPeerRemoteIP );
            ::lws_callback_on_writable( wsi );
        }
        break;
    case LWS_CALLBACK_CLOSED:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        if( self ) {
            self->onDisconnect( wsi, "NLWS: peer connection closed" );
        }
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        if( self ) {
            volatile bool bCloseAction = false;
            {  // block
                server_api::lock_type lock( self->mtx_api() );
                server_api::map_connections_t::iterator itCnFind = self->connections_.find( wsi ),
                                                        itCnEnd = self->connections_.end();
                if( itCnFind == itCnEnd )
                    return -1;  // this closes connection accoriding to
                // https://libwebsockets.org/lws-api-doc-master/html/md_README_8coding.html
                server_api::connection_data* pcd = itCnFind->second;
                if( !pcd )
                    return -1;  // this closes connection accoriding to
                // https://libwebsockets.org/lws-api-doc-master/html/md_README_8coding.html
                std::string cached_delayed_close_reason = pcd->delayed_close_reason_;
                lws_close_status cached_delayed_close_status = (lws_close_status) pcd->delayed_close_status_;
                std::string strDesc = pcd->description( false );
                std::string strDescC = pcd->description( true );
                pcd->delayed_close_reason_.clear();
                pcd->delayed_close_status_ = 0;
                //
                // self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, cc::debug("Processing writable state for
                // ") + strDescC );
                if( !cached_delayed_close_reason.empty() ) {
                    self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                                        cc::debug( "Processing close action for " ) + strDescC );
                    ::lws_close_reason( wsi,
                                        cached_delayed_close_status,
                                        (unsigned char*) cached_delayed_close_reason.c_str(),
                                        cached_delayed_close_reason.length() );
                    /// impl_lws_close_free_wsi( wsi, cached_delayed_close_status, 0 );
                    bCloseAction = true;
                    // self->onDisconnect( cid, msg );
                }  // if( ! cached_delayed_close_reason.empty() )
                else {
                    payload_queue_t& pq = pcd->buffer_;
                    while( !pq.empty() ) {
                        const message_payload_data& data = pq.front();
                        size_t cntLeft = 0;
                        size_t cntMaxPortion = self->cntMaxPortionBytesToSend_;

                        if( g_nDefaultBufferSizeTX > 0 && cntMaxPortion < g_nDefaultBufferSizeTX )
                            cntMaxPortion = g_nDefaultBufferSizeTX;
                        std::string strErrorDescription;
                        if( !data.send_to_wsi( wsi, cntMaxPortion, cntLeft, &strErrorDescription ) ) {
                            std::string strFailMessage;
                            strFailMessage += "NLWS: error writing to socket(server writable callback), pcd is ";
                            strFailMessage += strDesc;
                            if( !strErrorDescription.empty() ) {
                                strFailMessage += ", error description: ";
                                strFailMessage += strErrorDescription;
                            }
                            self->onFail( wsi, strFailMessage );
                            break;
                        }
                        if( cntLeft > 0 )
                            break;
                        pq.pop_front();  // only pop the message if it was sent successfully
                    }                    // while( ! pq.empty() )
                    if( pcd->delayed_adjustment_pong_timeout_ >=
                        0 ) {  // -1 for no adjustment, otherwise change pong timeout
                        int64_t timeout_pong = pcd->delayed_adjustment_pong_timeout_;
                        pcd->delayed_adjustment_pong_timeout_ = -1;
                        size_t i, cnt;
                        for( i = 0,
                             cnt = sizeof( g_arrPingPongTimeoutTypes ) / sizeof( g_arrPingPongTimeoutTypes[ 0 ] );
                             i < cnt;
                             ++i ) {
                            ::lws_set_timeout( wsi, g_arrPingPongTimeoutTypes[ i ], timeout_pong );
                        }
                    }
                }  // else from if( ! cached_delayed_close_reason.empty() )
            }      // block
            if( bCloseAction )
                return -1;  // this closes connection accoriding to
            // https://libwebsockets.org/lws-api-doc-master/html/md_README_8coding.html
        }  // if( self )
        break;

    case LWS_CALLBACK_RECEIVE:
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        if( self ) {
            message_payload_data data;
            if( impl_frame_is_binary( wsi, (uint8_t*) in, len ) )
                data.set_binary( (uint8_t*) in, len );
            else
                data.set_text( (char*) in, len );
            ////size_t nRemain = ::lws_remaining_packet_payload( wsi ); bool isFinalFragment = ( nRemain == 0 ) ? true :
            /// false;
            bool isFinalFragment = ::lws_is_final_fragment( wsi );
            self->onMessage( wsi, data, isFinalFragment );
        }
        break;
    case LWS_CALLBACK_HTTP: {
        //::lwsl_debug( "LWS_CALLBACK_HTTP\n" );
        ctx = ::lws_get_context( wsi );
        self = server_api::stat_get( ctx );
        unsigned int nHttpStatusToReturn = HTTP_STATUS_NOT_FOUND;
        std::string strHttpBodyToReturn = "<html><body>HTTP is not enabled on this server.</body></html>";
        if( self ) {
            nHttpStatusToReturn = self->nHttpStatusToReturn_;
            strHttpBodyToReturn = self->strHttpBodyToReturn_;
            self->onHttp( wsi );
        }
        ::lws_return_http_status( wsi, nHttpStatusToReturn, strHttpBodyToReturn.c_str() );
    } break;
        /*
                        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
                            ::lwsl_debug( "lws_http_client_http_response %d\n", ::lws_http_client_http_response(wsi) );
                            break;

                        // chunked content
                        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
                            ::lwsl_debug( "LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: %ld\n", (long)len );
        //					show_http_content( in, len );
                            break;

                        // unchunked content
                        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
                            {
        //						char buf[1024 + nPre];
        //						char *px = buf + nPre;
        //						int lenx = sizeof(buf) - nPre;
        //						// Often you need to flow control this by something else being writable. In that case
        call the api to get a callback when writable here,
        //						// and do the pending client read in the writeable callback of the output.
        //						// In the case of chunked content, this will call back
        LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ once per chunk or partial chunk in the buffer,
        //						// and report zero length back here.
        //						if( ::lws_http_client_read( wsi, &px, &lenx ) < 0 )
        //							return -1;
                            }
                        break;

                        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        //					strcpy( buf + nPre, "text=hello&send=Send+the+form" );
        //					n = ::lws_write( wsi, (unsigned char *)&buf[nPre], strlen(&buf[nPre]), LWS_WRITE_HTTP );
        //					if (n < 0)
        //						return -1;
        //					// we only had one thing to send, so inform lws we are done if we had more to send, call
        lws_callback_on_writable(wsi);
        //					// and just return 0 from callback.  On having sent the last part, call the below api
        instead.
        //					::lws_client_http_body_pending( wsi, 0 );
                        break;
        */

        /*
                        case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
                            // Verify the client certificate
                            if( (!len) || ( SSL_get_verify_result( (SSL*)in ) != X509_V_OK ) ) {
                                int err = X509_STORE_CTX_get_error( (X509_STORE_CTX*)user );
                                int depth = X509_STORE_CTX_get_error_depth( (X509_STORE_CTX*)user );
                                const char * msg = X509_verify_cert_error_string( err );
                                lwsl_err( "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION: SSL error: %s (%d),
        depth: %d\n", msg, err, depth ); return 1;
                            }
                        break;
        #if defined(LWS_HAVE_SSL_CTX_set1_param) && !defined(LWS_WITH_MBEDTLS)
                        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
                            if( crl_path[0] ) {
                                // Enable CRL checking
                                X509_VERIFY_PARAM * param = X509_VERIFY_PARAM_new();
                                X509_VERIFY_PARAM_set_flags( param, X509_V_FLAG_CRL_CHECK );
                                SSL_CTX_set1_param( (SSL_CTX*)user, param );
                                X509_STORE * store = SSL_CTX_get_cert_store( (SSL_CTX*)user );
                                X509_LOOKUP * lookup = X509_STORE_add_lookup( store, X509_LOOKUP_file() );
                                int n = X509_load_cert_crl_file( lookup, crl_path, X509_FILETYPE_PEM );
                                X509_VERIFY_PARAM_free( param );
                                if( n != 1 ) {
                                    char errbuf[256];
                                    n = ERR_get_error();
                                    lwsl_err( "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS: SSL error: %s
        (%d)\n", ERR_error_string( n, errbuf ), n ); return 1;
                                }
                            }
                        break;
        #endif
        */

        //				case LWS_CALLBACK_LOCK_POLL:
        //				case LWS_CALLBACK_UNLOCK_POLL:
        //				case LWS_CALLBACK_ADD_POLL_FD:
        //				case LWS_CALLBACK_DEL_POLL_FD:
        //				case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
        //				case LWS_CALLBACK_PROTOCOL_INIT:
        //				case LWS_CALLBACK_PROTOCOL_DESTROY:
        //				case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
        //				case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
        //				case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        //				case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
        //				case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
        //				case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
        //				case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        //				//case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
        //				case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
        //				{
        //					[](){}();
        //				}
        //				break;

    default:
        break;
    }  // switch( reason )
    return 0;
}
