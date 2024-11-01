srvmode_t str2srvmode( const std::string& s ) {
    std::string v = tiny::tools::to_lower( s );
    if( v.empty() )
        return srvmode_t::srvmode_simple;
    char c = v[ 0 ];
    if( c == 's' )
        return srvmode_t::srvmode_simple;
    if( c == 'p' )
        return srvmode_t::srvmode_external_poll;
#if( defined LWS_WITH_LIBEV )
    if( v == "ev" )
        return srvmode_t::srvmode_ev;
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    if( v == "event" )
        return srvmode_t::srvmode_event;
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    if( v == "uv" )
        return srvmode_t::srvmode_uv;
    if( v == "uvf" )
        return srvmode_t::srvmode_uv_with_foreign_loop;
#endif  // (defined LWS_WITH_LIBUV)
    return srvmode_t::srvmode_simple;
}
std::string srvmode2str( srvmode_t m ) {
    switch( m ) {
    case srvmode_t::srvmode_external_poll:
        return "poll";
#if( defined LWS_WITH_LIBEV )
    case srvmode_t::srvmode_ev:
        return "ev";
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    case srvmode_t::srvmode_event:
        return "event";
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    case srvmode_t::srvmode_uv:
        return "uv";
    case srvmode_t::srvmode_uv_with_foreign_loop:
        return "uvf";
#endif  // (defined LWS_WITH_LIBUV)
    case srvmode_t::srvmode_simple:
    default:
        return "simple";
    }  // switch( m )
}
std::string list_srvmodes_as_str() {
    std::string s;
    s += srvmode2str( srvmode_t::srvmode_simple );
    s += ", ";
    s += srvmode2str( srvmode_t::srvmode_external_poll );
#if( defined LWS_WITH_LIBEV )
    s += ", ";
    s += srvmode2str( srvmode_t::srvmode_ev );
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    s += ", ";
    s += srvmode2str( srvmode_t::srvmode_event );
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    s += ", ";
    s += srvmode2str( srvmode_t::srvmode_uv );
    s += ", ";
    s += srvmode2str( srvmode_t::srvmode_uv_with_foreign_loop );
#endif  // (defined LWS_WITH_LIBUV)
    return s;
}

srvmode_t g_default_srvmode = srvmode_t::
#if( defined LWS_WITH_LIBUV )
    srvmode_uv;
// srvmode_uv_with_foreign_loop;
#else
#if( defined LWS_WITH_LIBEV )
    srvmode_ev;
#else
#if( defined LWS_WITH_LIBEVENT )
    srvmode_event;
#else
    srvmode_simple;
#endif
#endif
#endif

bool g_default_explicit_vhost_enable = true;  // srvmode_simple and srvmode_external_poll only
bool g_default_dynamic_vhost_enable = false;

server_vhost_settings::server_vhost_settings( const nlws::server_vhost_settings& r )
: isSSL( r.isSSL )
, port( r.port.load() )
, sa( r.sa )
, uri( r.uri ) {}
server_vhost_settings::server_vhost_settings( security_args sa, int port, std::string uri )
: isSSL( !sa.strCertificateFile_.empty() )
, port( port )
, sa( std::move( sa ) )
, uri( std::move( uri ) ) {}

server_api::connection_data::connection_data() {}
server_api::connection_data::~connection_data() {
    setPeer( nullptr );
}
peer_ptr_t server_api::connection_data::getPeer() {
    return pPeer_;
}
void server_api::connection_data::setPeer( peer_ptr_t pPeer ) {
    if( pPeer_ == pPeer )
        return;
    if( pPeer_ ) {
        pPeer_->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                              cc::debug( "Server API entry destruction for " ) + description( true ) );
        pPeer_->ref_release();
        pPeer_ = nullptr;
        sn_ = 0;
    }
    if( pPeer ) {
        pPeer_ = pPeer;
        pPeer_->ref_retain();
        sn_ = pPeer_->serial_number();
        pPeer_->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                              cc::debug( "Server API entry construction for " ) + description( true ) );
    }
}
std::string server_api::connection_data::unique_string_identifier( bool isColored /*= false*/ ) const {
    std::string strCid( peer::stat_getCidString( cid_ ) ), strSlash( "/" );
    if( isColored ) {
        strSlash = cc::debug( strSlash );
        if( !strCid.empty() )
            strCid = cc::bright( strCid );
    }
    std::string strPeerSerialNumber = tiny::tools::format( "%" PRIu64, uint64_t( sn_ ) );
    if( isColored )
        strPeerSerialNumber = cc::notice( strPeerSerialNumber );
    std::stringstream ss;
    ss << strCid << strSlash << strPeerSerialNumber;
    return ss.str();
}
std::string server_api::connection_data::description( bool isColored /*= false*/ ) const {
    std::string strEq( "=" ), strSeparator( ", " ), strNameUis( "uis" ), strNameIP( "ip" ),
        strValueIP( strPeerRemoteIP_ ), strNameCloseStatus( "close status" ),
        strValueCloseStatus( tiny::tools::format( "%d", delayed_close_status_ ) ), strNameCloseReason( "close reason" ),
        strValueCloseReason( isColored ? cc::warn( delayed_close_reason_ ) : delayed_close_reason_ );
    if( isColored ) {
        strEq = cc::debug( strEq );
        strSeparator = cc::debug( strSeparator );
        strNameUis = cc::notice( strNameUis );
        strNameIP = cc::notice( strNameIP );
        strNameCloseStatus = cc::notice( strNameCloseStatus );
        strNameCloseReason = cc::notice( strNameCloseReason );
        strValueIP = cc::u( strValueIP );
        strValueCloseStatus = cc::warn( strValueCloseStatus );
    }
    std::stringstream ss;
    ss << strNameUis << strEq << unique_string_identifier( isColored );
    if( !strPeerRemoteIP_.empty() )
        ss << strSeparator << strNameIP << strEq << strValueIP;
    if( delayed_close_status_ != 0 )
        ss << strSeparator << strNameCloseStatus << strEq << strValueCloseStatus;
    if( !delayed_close_reason_.empty() )
        ss << strSeparator << strNameCloseReason << strEq << strValueCloseReason;
    return ss.str();
}

server_api::server_api() {
    clear_fields();
}
server_api::~server_api() {
    deinit();
}

void server_api::clear_fields() {
    basic_api::clear_fields();
    overall_connections_count_ = 0;
    ctx_info_.port = 443;
    vec_lws_protocols_[ default_protocol_index_ ].callback = stat_callback_server;
    //
    srvmode_ = g_default_srvmode;
    explicit_vhost_enable_ = g_default_explicit_vhost_enable;
    //
    external_poll_max_elements_ = 0;
    external_poll_fds_ = nullptr;
    external_poll_fd_lookup_ = nullptr;
    external_poll_cnt_fds_ = 0;
    ::memset( &external_poll_tv_, 0, sizeof( struct timeval ) );
    external_poll_ms_1_second_ = 0;
    dynamic_vhost_enable_ = g_default_dynamic_vhost_enable;
    dynamic_vhost_ = nullptr;
    //
    //::memset( &fops_plat_, 0, sizeof(lws_plat_file_ops) );
    //
    vhost_ = nullptr;
    //				interface_name_.clear();
    external_poll_ms_ = external_poll_oldms_ = 0;
    //				iface_ = nullptr;
    cert_path_.clear();
    key_path_.clear();
    ca_path_.clear();
    use_ssl_ = false;
    //
    serverInterruptFlag_ = false;
    fn_internal_interrupt_action_ = nullptr;
    //
    nHttpStatusToReturn_ = HTTP_STATUS_NOT_FOUND;
    strHttpBodyToReturn_ = "<html><body>HTTP is not enabled on this server.</body></html>";

    vhosts_custom_.clear();
}

bool server_api::appendVHost( server_vhost_settings& vhostSettings ) {
    if( !initialized_ )
        return false;

    lock_type lock( mtx_api() );

    // handleVirtualHostsCreate( &ctx_info_ );
    /** Open the same service on another port and on http (not secure). */
    server_vhost_entry_t newVHost = { vhostSettings, nullptr };

    auto vhostInfo = ctx_info_;
    if( !vhostSettings.isSSL ) {
        vhostInfo.ssl_cert_filepath = nullptr;
        vhostInfo.ssl_private_key_filepath = nullptr;
        vhostInfo.ssl_ca_filepath = nullptr;
        vhostInfo.options &=
            ~( LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE );
    }
    else {
        auto valueOrNull = []( const std::string& value ) { return value.empty() ? value.data() : nullptr; };
        vhostInfo.ssl_cert_filepath = valueOrNull( vhostSettings.sa.strCertificateFile_ );
        vhostInfo.ssl_private_key_filepath = valueOrNull( vhostSettings.sa.strPrivateKeyFile_ );
        vhostInfo.ssl_ca_filepath = valueOrNull( vhostSettings.sa.strCertificationChainFile_ );
        vhostInfo.options |=
            ( LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE );
    }
    vhostInfo.port = vhostSettings.port;

    auto anotherVHost = ::lws_create_vhost( ctx_, &vhostInfo );
    if( anotherVHost ) {
        if( vhostSettings.port == 0 ) {
            vhostSettings.port = ::lws_get_vhost_port( anotherVHost );
            newVHost.first.port = vhostSettings.port.load();
            newVHost.second = anotherVHost;
        }

        if( vhostSettings.isSSL )
            ::lws_init_vhost_client_ssl( &vhostInfo, anotherVHost );

        onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                      tiny::tools::format( "vhost created: port=\"%d\", isSsl=%s",
                                           newVHost.first.port.load(),
                                           cc::flag( newVHost.first.isSSL ).c_str() ) );

        vhosts_custom_.push_back( std::move( newVHost ) );

        return true;
    }

    return false;
}

bool server_api::init( bool isSSL, int& nPort, security_args* pSA, const lws_context_configuration& configuration ) {
    if( initialized_ )
        return false;

    lock_type lock( mtx_api() );

    deinit();

    if( server_disable_ipv6_ )
        ctx_info_.options |= LWS_SERVER_OPTION_DISABLE_IPV6;
    if( server_validate_utf8_ )
        ctx_info_.options |= LWS_SERVER_OPTION_VALIDATE_UTF8;
    if( server_skip_canonical_name_ )
        ctx_info_.options |= LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;

    use_ssl_ = isSSL;
    if( use_ssl_ && pSA != nullptr ) {
        cert_path_ = pSA->strCertificateFile_;
        key_path_ = pSA->strPrivateKeyFile_;
        ca_path_ = pSA->strCertificationChainFile_;  // ???
        use_ssl_ = true;

        if( ssl_perform_global_init_ )
            ctx_info_.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

        if( ssl_server_require_valid_certificate_ )
            ctx_info_.options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
        if( ssl_server_allow_non_ssl_on_ssl_port_ )
            ctx_info_.options |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
        if( ssl_server_accept_connections_without_valid_cert_ )
            ctx_info_.options |= LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
        if( ssl_server_redirect_http_2_https_ )
            ctx_info_.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;
        //| LWS_SERVER_OPTION_CREATE_VHOST_SSL_CTX // looks like we never need this
        //| LWS_SERVER_OPTION_DISABLE_OS_CA_CERTS // looks like we never need this
        //
        //
        ctx_info_.ssl_cert_filepath = ( !cert_path_.empty() ) ? cert_path_.c_str() : nullptr;
        ctx_info_.ssl_private_key_filepath = ( !key_path_.empty() ) ? key_path_.c_str() : nullptr;
        ctx_info_.ssl_ca_filepath = ( !ca_path_.empty() ) ? ca_path_.c_str() : nullptr;
#if defined( LWS_HAVE_SSL_CTX_set1_param )
        //					crl_path_ = ...
#endif
        //
        //
        onLogMessage(
            e_ws_log_message_type_t::eWSLMT_debug,
            tiny::tools::format( "ctx_info_.ssl_cert_filepath=\"%s\"    ctx_info_.ssl_private_key_filepath=\"%s\"   "
                                 "ctx_info_.ssl_ca_filepath=\"%s\"",
                                 ctx_info_.ssl_cert_filepath,
                                 ctx_info_.ssl_private_key_filepath,
                                 ctx_info_.ssl_ca_filepath ) );
    }  // if( use_ssl_ && pSA != nullptr )

    switch( srvmode_ ) {
#if( defined LWS_WITH_LIBEV )
    case srvmode_t::srvmode_ev:
        ctx_info_.options |= LWS_SERVER_OPTION_LIBEV;
        break;
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    case srvmode_t::srvmode_event:
        ctx_info_.options |= LWS_SERVER_OPTION_LIBEVENT;
        break;
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    case srvmode_t::srvmode_uv:
    case srvmode_t::srvmode_uv_with_foreign_loop:
        ctx_info_.options |= LWS_SERVER_OPTION_LIBUV;
        break;
#endif  // (defined LWS_WITH_LIBUV)
    default:
        if( explicit_vhost_enable_ )
            ctx_info_.options |= LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
        break;
    }  // switch( srvmode_ )

    ctx_info_.port = nPort;
    ctx_info_.timeout_secs_ah_idle = configuration.ahIdleTimeout.count();

    //				interface_name_ = optarg;
    //				iface = interface_name.c_str(); // ???

    //				ctx_info_.bind_iface = 1;
    //				#if defined( LWS_HAVE_SYS_CAPABILITY_H) && defined(LWS_HAVE_LIBCAP)
    //							ctx_info_.caps[0] = CAP_NET_RAW;
    //							ctx_info_.count_caps = 1;
    //				#endif

    //				close_testing = 1;
    //				self->onLogMessage( e_ws_log_message_type_t::eWSLMT_debug, "NLWS: close testing mode -- closes on
    // client after 50 dumb incrementsand suppresses lws_mirror spam");

    //				::signal( SIGINT, sighandler );
    //				::signal( SIGUSR1, sighandler );
    // we will only try to log things according to our debug_level
    //::setlogmask( LOG_UPTO (LOG_DEBUG) );
    //::openlog( "lwsts", syslog_options, LOG_DAEMON );

    //::lwsl_notice( "libwebsockets test server - license LGPL2.1+SLE\n" );
    //::lwsl_notice( "(C) Copyright 2010-2017 Andy Green <andy@warmcat.com>\n" );

    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll: {
        external_poll_max_elements_ = size_t(::getdtablesize() );
        size_t cntBytesToAlloc = external_poll_max_elements_ * sizeof( struct lws_pollfd );
        external_poll_fds_ = (lws_pollfd*) ::malloc( cntBytesToAlloc );
        if( external_poll_fds_ == nullptr ) {
            ::lwsl_err( "Out of memory while allocating %d bytes of memory for %d elements in external_poll_fds_\n",
                        int( cntBytesToAlloc ),
                        int( external_poll_max_elements_ ) );
            return false;
        }
        ::memset( external_poll_fds_, 0, cntBytesToAlloc );
        cntBytesToAlloc = external_poll_max_elements_ * sizeof( int );
        external_poll_fd_lookup_ = (int*) ::malloc( cntBytesToAlloc );
        if( external_poll_fd_lookup_ == nullptr ) {
            ::free( external_poll_fds_ );
            external_poll_fds_ = nullptr;
            ::lwsl_err(
                "Out of memory while allocating %d bytes of memory for %d elements in external_poll_fd_lookup_\n",
                int( cntBytesToAlloc ),
                int( external_poll_max_elements_ ) );
            return false;
        }
        ::memset( external_poll_fd_lookup_, 0, cntBytesToAlloc );
    } break;
    default:
        break;
    }  // switch( srvmode_ )

    //				ctx_info_.iface = iface_;
    if( interval_ping_ > 0 )
        ctx_info_.ws_ping_pong_interval = interval_ping_;
    ctx_info_.max_http_header_pool = 256;

    ctx_info_.user = this;

    ctx_info_.ssl_cipher_list = nullptr
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

    //				info.ip_limit_ah = 24;   // for testing
    //				info.ip_limit_wsi = 105; // for testing

    beforeCreateContext();

    // TODO: log ctx_info_
    onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                  tiny::tools::format( "ctx_info_.timeout_secs_ah_idle=%d", ctx_info_.timeout_secs_ah_idle ) );

    ctx_ = ::lws_create_context( &ctx_info_ );
    if( !ctx_ ) {
        lwsl_err( "libwebsockets init failed\n" );
        onLogMessage( e_ws_log_message_type_t::eWSLMT_error,
                      tiny::tools::format( "server_api::init: ::lws_create_context( &ctx_info_ ) failed, error: %s",
                                           strerror( errno ) ) );

        deinit();
        return false;
    }
    stat_reg( this );

    //				/////lws_uv_initloop( ctx_, &loop,  0);
    //				if( ::lws_uv_initloop( ctx_, nullptr, 0 ) ) {
    //					deinit();
    //					onLogMessage( e_ws_log_message_type_t::eWSLMT_error, "NLWS: lws_uv_initloop() failed for server"
    //); 					return false;
    //				}

    errno = 0;
    switch( srvmode_ ) {
    case srvmode_t::srvmode_simple:
    case srvmode_t::srvmode_external_poll: {
        vhost_ = ::lws_create_vhost( ctx_, &ctx_info_ );
        if( !vhost_ ) {
            lwsl_err( "vhost creation failed\n" );
            onLogMessage(
                e_ws_log_message_type_t::eWSLMT_error,
                tiny::tools::format(
                    "server_api::init: vhost creation on port %d failed: %s", ctx_info_.port, strerror( errno ) ) );

            deinit();
            return false;
        }

        // For testing dynamic vhost create / destroy later, we use port + 1
        // Normally if you were creating more vhosts, you would set info.name
        // for each to be the hostname external clients use to reach it
        // // // //ctx_info_.port++;

        ::lws_init_vhost_client_ssl( &ctx_info_, vhost_ );
        if( nPort == 0 ) {
            nPort = ::lws_get_vhost_port( vhost_ );
            onLogMessage( e_ws_log_message_type_t::eWSLMT_debug,
                          tiny::tools::format( "server_api::init: explicit vhost port = %d", nPort ) );
        }
    } break;
    default:
        dynamic_vhost_enable_ = false;
        break;
    }  // switch( srvmode_ )

    // this shows how to override the lws file operations.	You don't need
    // to do any of this unless you have a reason (eg, want to serve
    // compressed files without decompressing the whole archive)

    //				// stash original platform fops
    //				fops_plat_ = *(::lws_get_fops(ctx_));
    //				// override the active fops
    //				::lws_get_fops(ctx_)->open = test_server_fops_open;

    ::memset( &external_poll_tv_, 0, sizeof( struct timeval ) );
    ::gettimeofday( &external_poll_tv_, nullptr );
    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll:
        external_poll_ms_1_second_ = 0;
        break;
    default:
        break;
    }  // switch( srvmode_ )
    // do_writable_callbacks_all_protocol();
    initialized_ = true;
    return true;
}
void server_api::deinit() {
    lock_type lock( mtx_api() );

    if( dynamic_vhost_ ) {
        ::lws_vhost_destroy( dynamic_vhost_ );
        dynamic_vhost_ = nullptr;
    }
    for( auto& setting : vhosts_custom_ ) {
        auto& vhostPtr = setting.second;
        if( vhostPtr ) {
            ::lws_vhost_destroy( vhostPtr );
            vhostPtr = nullptr;
        }
    }

    if( ctx_ ) {
        auto ctx2destroy = ctx_.load();
        ctx_ = nullptr;
        ::lws_context_destroy( ctx2destroy );
    }
    stat_unreg( this );
    if( external_poll_fds_ ) {
        ::free( external_poll_fds_ );
        external_poll_fds_ = nullptr;
    }
    if( external_poll_fd_lookup_ ) {
        ::free( external_poll_fd_lookup_ );
        external_poll_fd_lookup_ = nullptr;
    }
    initialized_ = false;

    map_connections_t::iterator itCnWalk = connections_.begin(), itCnEnd = connections_.end();
    for( ; itCnWalk != itCnEnd; ++itCnWalk ) {
        connection_data* pcd = itCnWalk->second;
        if( pcd )
            delete pcd;
    }
    connections_.clear();

    clear_fields();
}
void server_api::close( connection_identifier_t cid, int nCloseStatus, const std::string& msg ) {
    if( !initialized_ )
        return;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return;
    server_api::connection_data* pcd = itCnFind->second;
    if( !pcd )
        return;
    struct lws* wsi = pcd->wsi_;
    if( wsi == nullptr )
        return;
    pcd->delayed_close_reason_ = msg;
    pcd->delayed_close_status_ = nCloseStatus;
}

void server_api::external_poll_service_loop( fn_continue_status_flag_t fnContinueStatusFlag,
                                             size_t nServiceLoopRunLimit /*= 0*/ ) {
    if( fnContinueStatusFlag ) {
        if( !fnContinueStatusFlag() )
            return;
    }
    int n = 0;
    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll:
        external_poll_ms_1_second_ = 0;
        break;
    default:
        lwsl_debug( "server_api::external_poll_service_loop() is unavailable!\n" );
        break;
    }  // switch( srvmode_ )
    for( size_t idxLoop = 0; n >= 0; ++idxLoop ) {
        if( nServiceLoopRunLimit > 0 && idxLoop >= nServiceLoopRunLimit )
            break;
        if( fnContinueStatusFlag ) {
            if( !fnContinueStatusFlag() )
                return;
        }
        ::memset( &external_poll_tv_, 0, sizeof( struct timeval ) );
        ::gettimeofday( &external_poll_tv_, nullptr );
        // this provokes the LWS_CALLBACK_SERVER_WRITEABLE for every live websocket connection using the our default
        // protocol, as soon as it can take more packets (usually immediately)
        external_poll_ms_ = ( external_poll_tv_.tv_sec * 1000 ) + ( external_poll_tv_.tv_usec / 1000 );
        if( ( external_poll_ms_ - external_poll_oldms_ ) > 50 ) {
            do_writable_callbacks_all_protocol();
            external_poll_oldms_ = external_poll_ms_;
        }
        switch( srvmode_ ) {
        case srvmode_t::srvmode_external_poll: {
            // this represents an existing server's single poll action which also includes libwebsocket sockets
            if( !service_poll( n ) )
                return;
            if( n < 0 )
                continue;
        } break;
        default: {
            // if libwebsockets sockets are all we care about, you can use this api which takes care of the poll() and
            // looping through finding who needed service if no socket needs service, it'll return anyway after the
            // number of ms in the second argument
            n = ::lws_service( ctx_, 50 );
        } break;
        }  // switch( srvmode_ )
        //
        if( dynamic_vhost_enable_ && ( !dynamic_vhost_ ) ) {
            ::lwsl_notice( "creating dynamic vhost...\n" );
            dynamic_vhost_ = ::lws_create_vhost( ctx_, &ctx_info_ );
        }
        else if( ( !dynamic_vhost_enable_ ) && dynamic_vhost_ ) {
            ::lwsl_notice( "destroying dynamic vhost...\n" );
            ::lws_vhost_destroy( dynamic_vhost_ );
            dynamic_vhost_ = nullptr;
        }
    }  // for( size_t idxLoop = 0; ...
}
bool server_api::service_poll( int& n ) {  // if returns true - continue service loop
    if( !initialized_ )
        return false;
    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll: {
        n = ::poll( external_poll_fds_, external_poll_cnt_fds_, 50 );
        if( n < 0 )
            return true;
        if( n ) {
            for( n = 0; n < external_poll_cnt_fds_; ++n ) {
                if( external_poll_fds_[ n ].revents )
                    // returns immediately if the fd does not match anything under libwebsockets control
                    if(::lws_service_fd( ctx_, &external_poll_fds_[ n ] ) < 0 )
                        return false;
            }
            // if needed, force-service wsis that may not have read all input
            while( !::lws_service_adjust_timeout( ctx_, 1, 0 ) ) {
                lwsl_debug( "extpoll doing forced service!\n" );
                ::lws_service_tsi( ctx_, -1, 0 );
            }
        }
        else {
            // no revents, but before polling again, make lws check for any timeouts
            if( external_poll_ms_ - external_poll_ms_1_second_ > 1000 ) {
                lwsl_debug( "1 per sec\n" );
                ::lws_service_fd( ctx_, nullptr );
                external_poll_ms_1_second_ = external_poll_ms_;
            }
        }
    } break;
    default:
        ::lws_service( ctx_, 50 );
        break;
    }  // switch( srvmode_ )
    return true;
}

void server_api::poll( fn_continue_status_flag_t fnContinueStatusFlag ) {
    switch( srvmode_ ) {
    case srvmode_t::srvmode_simple: {
        int n = 0;
        service_poll( n );
    } break;
    case srvmode_t::srvmode_external_poll:
        if( fnContinueStatusFlag && fnContinueStatusFlag() )
            external_poll_service_loop( fnContinueStatusFlag, 10 );
        break;
    default:
        lwsl_debug( "server_api::poll() is unavailable!\n" );
        break;
    }  // switch( srvmode_ )
}

#if( defined LWS_WITH_LIBEV )
static void my_ev_timeout_cb( EV_P_ ev_timer* w, int revents ) {
    server_api* self = server_api::stat_ptr2api( w );
    if( !self ) {
        ::lwsl_err( "my_ev_timeout_cb() failed to get server_api\n" );
        return;
    }
    if( self->serverInterruptFlag_ ) {
        ::ev_break( EV_A_ EVBREAK_ONE );
        return;
    }
    //::lws_callback_on_writable_all_protocol( self->ctx_, &protocols[PROTOCOL_DUMB_INCREMENT] );
    self->do_writable_callbacks_all_protocol();
}
#endif  // (defined LWS_WITH_LIBEV)

#if( defined LWS_WITH_LIBEVENT )
static void my_event_ev_timeout_cb( evutil_socket_t sock_fd, short events, void* ctx ) {
    server_api* self = (server_api*) ctx;
    if( !self ) {
        ::lwsl_err( "my_event_ev_timeout_cb() failed to get server_api\n" );
        return;
    }
    if( self->serverInterruptFlag_ ) {
        return;
    }
    //::lws_callback_on_writable_all_protocol( context, &protocols[PROTOCOL_DUMB_INCREMENT] );
    self->do_writable_callbacks_all_protocol();
}
#endif  // (defined LWS_WITH_LIBEVENT)

#if( defined LWS_WITH_LIBUV )
struct my_uv_counter_struct {
    server_api& m_clientApi;
    volatile size_t cur_ = 0;
    volatile size_t lim_ = 65535;
    volatile bool stop_loop_ = false;
    my_uv_counter_struct( server_api& api )
    : m_clientApi( api ) {}
};  /// struct my_uv_counter_struct
    //			static void my_uv_signal_cb( uv_signal_t * watcher, int signum ) {
    //				//struct my_uv_counter_struct * c = (struct my_uv_counter_struct *)t->data;
    //				//server_api & self = c->m_clientApi;
    //				//self.serverInterruptFlag_ = true;
    //				::lwsl_err( "Signal %d caught, exiting...\n", watcher->signum );
    //				switch( watcher->signum ) {
    //				case SIGTERM:
    //				case SIGINT:
    //					break;
    //				default:
    //					::signal( SIGABRT, SIG_DFL );
    //					::abort();
    //					break;
    //				}
    //				if( self.ctx_ ) {
    //					lwsl_debug( "my_uv_signal_cb() will cancel lws service\n" );
    //					::lws_cancel_service( self.ctx_ );
    //				}
    //			}
static void my_uv_stopping_timer_cb( uv_timer_t* t ) {
    struct my_uv_counter_struct* c = (struct my_uv_counter_struct*) t->data;
    server_api& self = c->m_clientApi;
    if( self.serverInterruptFlag_ ) {
        if( self.ctx_ )
            ::lws_libuv_stop( self.ctx_ );
        ::uv_stop( t->loop );
        return;
    }
}
static void my_uv_timer_cb( uv_timer_t* t ) {
    struct my_uv_counter_struct* c = (struct my_uv_counter_struct*) t->data;
    server_api& self = c->m_clientApi;
    if( self.serverInterruptFlag_ ) {
        if( self.ctx_ )
            ::lws_libuv_stop( self.ctx_ );
        ::uv_stop( t->loop );
        return;
    }
    lwsl_debug( "  timer %p cb, count %d, loop has %d handles\n", t, int( c->cur_ ), t->loop->active_handles );
    if( c->cur_++ >= c->lim_ ) {
        lwsl_debug( "stop loop from timer\n" );
        ::uv_timer_stop( t );
        if( c->stop_loop_ )
            ::uv_stop( t->loop );
    }  // if( c->cur_++ >= c->lim_ )
}
static void my_uv_timer_test_cancel_cb( uv_timer_t* t ) {
    struct my_uv_counter_struct* c = (struct my_uv_counter_struct*) t->data;
    if( c == nullptr ) {
        ::uv_stop( t->loop );
        return;
    }
    server_api& self = c->m_clientApi;
    if( self.serverInterruptFlag_ ) {
        if( self.ctx_ )
            ::lws_libuv_stop( self.ctx_ );
        ::uv_stop( t->loop );
        return;
    }
    // if( self.ctx_ )
    //	::lws_cancel_service( self.ctx_ );
}
static void my_uv_timer_close_cb( uv_handle_t* h ) {
    ::lwsl_notice( "timer close cb %p, loop has %d handles\n", h, h->loop->active_handles );
}
//			static void my_uv_outer_signal_cb( uv_signal_t * s, int signum ) {
//				::lwsl_notice( "Foreign loop got signal %d\n", signum );
//				::uv_signal_stop( s );
//				::uv_stop( s->loop );
//			}
static void my_uv_lws_uv_close_cb( uv_handle_t* handle ) {
    //::lwsl_err( "%s\n", __func__ );
}
static void my_uv_lws_uv_walk_cb( uv_handle_t* handle, void* arg ) {
    ::uv_close( handle, my_uv_lws_uv_close_cb );
}
static void my_uv_idle_cb( uv_idle_t* uvi ) {
    server_api* self = (server_api*) uvi->data;
    if( self->serverInterruptFlag_ ) {
        if( self->ctx_ )
            ::lws_libuv_stop( self->ctx_ );
        else
            return;
    }
}
#endif  // (defined LWS_WITH_LIBUV)

bool server_api::service_mode_supported() const {
    if( !initialized_ )
        return false;
    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll:
#if( defined LWS_WITH_LIBEV )
    case srvmode_t::srvmode_ev:
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    case srvmode_t::srvmode_event:
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    case srvmode_t::srvmode_uv:
    case srvmode_t::srvmode_uv_with_foreign_loop:
#endif  // (defined LWS_WITH_LIBUV)
        return true;
    default:
        return false;
    }  // switch( srvmode_ )
}
void server_api::service_interrupt() {
    if( !initialized_ )
        return;

    try {
        if( fn_internal_interrupt_action_ )
            fn_internal_interrupt_action_();
    }
    catch( ... ) {
    }
    switch( srvmode_ ) {
    case srvmode_t::srvmode_simple:
    case srvmode_t::srvmode_external_poll: {
        // TO-CHECK: whether this works as server interrupt
        if( ctx_ )
            ::lws_cancel_service( ctx_ );

        //::lws_context_destroy( ctx_ );

        // lock_type lock( mtx_api() );
        // deinit();
    }
        return;
    default:
        lwsl_debug( "server_api::service() is unavailable!\n" );
        return;
    }  // switch( srvmode_ )
}
void server_api::service( fn_continue_status_flag_t fnContinueStatusFlag ) {
    if( !initialized_ )
        return;
    switch( srvmode_ ) {
    case srvmode_t::srvmode_external_poll:
        external_poll_service_loop( fnContinueStatusFlag );
        return;
#if( defined LWS_WITH_LIBEV )
    case srvmode_t::srvmode_ev: {
        // notice: fnContinueStatusFlag is not used here
        struct ev_loop* loop = ::ev_default_loop( 0 );
        fn_internal_interrupt_action_ = [&]() {
            serverInterruptFlag_ = true;
            if( loop != nullptr )
                ::ev_break( loop, EVBREAK_ONE );
        };
        ev_timer timeout_watcher;

        //						for( n = 0; n < (int)ARRAY_SIZE(sigs); n++ ) {
        //							ev_init(&signals[n], signal_cb);
        //							ev_signal_set(&signals[n], sigs[n]);
        //							ev_signal_start(loop, &signals[n]);
        //						}

        server_api::stat_ptr_reg( &timeout_watcher, this );
        ::lws_ev_initloop( ctx_, loop, 0 );
        ev_timer_init( &timeout_watcher, my_ev_timeout_cb, 0.05, 0.05 );
        ::ev_timer_start( loop, &timeout_watcher );
        ev_run( loop, 0 );
        server_api::stat_ptr_unreg( &timeout_watcher );
    }
        return;
#endif  // (defined LWS_WITH_LIBEV)
#if( defined LWS_WITH_LIBEVENT )
    case srvmode_t::srvmode_event: {
        // notice: fnContinueStatusFlag is not used here
        struct event_base* event_base_loop = ::event_base_new();
        fn_internal_interrupt_action_ = [&]() {
            serverInterruptFlag_ = true;
            if( event_base_loop != nullptr )
                ::event_base_loopbreak( event_base_loop );
        };
        struct event* timeout_watcher = nullptr;
        // do not use the default Signal Event Watcher & Handler
        ::lws_event_sigint_cfg( ctx_, 0, nullptr );
        // initialize the LWS with libevent loop
        ::lws_event_initloop( ctx_, event_base_loop, 0 );
        //
        timeout_watcher = ::event_new( event_base_loop, -1, EV_PERSIST, my_event_ev_timeout_cb, this );
        struct timeval tv = { 0, 50000 };
        ::evtimer_add( timeout_watcher, &tv );
        ::event_base_dispatch( event_base_loop );
    }
        return;
#endif  // (defined LWS_WITH_LIBEVENT)
#if( defined LWS_WITH_LIBUV )
    case srvmode_t::srvmode_uv:
    case srvmode_t::srvmode_uv_with_foreign_loop: {
        // notice: fnContinueStatusFlag is not used here
        bool foreign_libuv_loop = ( srvmode_ == srvmode_t::srvmode_uv_with_foreign_loop ) ? true : false;
        uv_loop_t loop;
        ::memset( &loop, 0, sizeof( uv_loop_t ) );
        // uv_idle_t idler;
        //::memset( &idler, 0, sizeof(uv_idle_t) );

        // uv_signal_t signal_outer;
        //::memset( &signal_outer, 0, sizeof(uv_signal_t) );

        uv_timer_t timer_outer;
        ::memset( &timer_outer, 0, sizeof( uv_timer_t ) );

        uv_timer_t timer_test_cancel;
        ::memset( &timer_test_cancel, 0, sizeof( uv_timer_t ) );

        my_uv_counter_struct ctr( *this );

        fn_internal_interrupt_action_ = [&]() {
            serverInterruptFlag_ = true;
            if( ctx_ )
                ::lws_libuv_stop( ctx_ );
            ::uv_stop( &loop );
        };

        //
        // next part is usually invoked before lws context creation
        //

#if UV_VERSION_MAJOR > 0
        if( foreign_libuv_loop ) {
            ::uv_loop_init( &loop );  // create the foreign loop
            // run some timer on that loop just so loop is not 'clean'
            ::uv_timer_init( &loop, &timer_test_cancel );
            ::uv_timer_start( &timer_test_cancel, my_uv_timer_test_cancel_cb, 200, 200 );  // TO-CHECK: was 2000, 2000

            ::uv_timer_init( &loop, &timer_outer );
            timer_outer.data = &ctr;
            ctr.cur_ = 0;
            ctr.lim_ = ctr.cur_ + 5;  // TO-CHECK: wth is this?
            ctr.stop_loop_ = true;
            ::uv_timer_start( &timer_outer, my_uv_timer_cb, 0, 1000 );
            ::lwsl_notice( "running loop without libwebsockets for %d s\n", int( ctr.lim_ ) );

            ::uv_run( &loop, UV_RUN_DEFAULT );
            fn_internal_interrupt_action_ = nullptr;
            // timer will stop loop and we will get here
        }
#endif

        //
        // next part is usually invoked after lws context creation
        //

        //::lws_uv_sigint_cfg( ctx_, 1, my_uv_signal_cb );
#if UV_VERSION_MAJOR > 0
        if( foreign_libuv_loop ) {
            ::lws_uv_initloop( ctx_, &loop, 0 );  // we have our own uv loop outside of lws
        }
        else
#endif
        {  // lws will create his own libuv loop in the context
            if(::lws_uv_initloop( ctx_, nullptr, 0 ) ) {
                ::lwsl_err( "lws_uv_initloop() failed\n" );
                goto bail;
            }
        }
#if UV_VERSION_MAJOR > 0
        if( foreign_libuv_loop ) {
            // prepare inner timer on loop, to run along with lws. Will exit after 5s while lws keeps running
            my_uv_counter_struct ctr_inner( *this );  //= { 0, 3, 0 };
            int e;
            uv_timer_t timer_inner;
            ::uv_timer_init( &loop, &timer_inner );
            timer_inner.data = &ctr_inner;
            ::uv_timer_start( &timer_inner, my_uv_timer_cb, 200, 1000 );
            // make this timer long-lived, should keep firing after lws exits
            ctr.cur_ = 0;
            ctr.lim_ = ctr.cur_ + 1000;  // TO-CHECK: wth is this?
            ::uv_timer_start( &timer_outer, my_uv_timer_cb, 0, 1000 );
            //
            ::uv_run( &loop, UV_RUN_DEFAULT );
            fn_internal_interrupt_action_ = nullptr;
            // we are here either because signal stopped us, or outer timer expired
            //
            // close short timer
            ::uv_timer_stop( &timer_inner );
            ::uv_close( (uv_handle_t*) &timer_inner, my_uv_timer_close_cb );

            ::lwsl_notice( "Destroying lws context\n" );
            ::lws_context_destroy( ctx_ );  // detach lws

            lwsl_notice( "Please wait while the outer libuv test continues for 10s\n" );
            ctr.lim_ = ctr.cur_ + 10;  // TO-CHECK: wth is this?

            // try and run outer timer for 10 more seconds, (or sigint outer handler) after lws has left the loop
            ::uv_run( &loop, UV_RUN_DEFAULT );
            fn_internal_interrupt_action_ = nullptr;

            // clean up the foreign loop now...
            // PHASE 1: stop and close things we created outside of lws
            ::uv_timer_stop( &timer_outer );
            ::uv_timer_stop( &timer_test_cancel );
            ::uv_close( (uv_handle_t*) &timer_outer, my_uv_timer_close_cb );
            //::uv_signal_stop( &signal_outer );
            e = 100;
            while( e-- ) {
                ::uv_run( &loop, UV_RUN_NOWAIT );
                fn_internal_interrupt_action_ = nullptr;
            }
            // PHASE 2: close anything remaining
            ::uv_walk( &loop, my_uv_lws_uv_walk_cb, nullptr );
            e = 100;
            while( e-- ) {
                ::uv_run( &loop, UV_RUN_NOWAIT );
                fn_internal_interrupt_action_ = nullptr;
            }
            // PHASE 3: close the UV loop itself
            e = ::uv_loop_close( &loop );
            ::lwsl_notice( "uv loop close rc %s\n", e ? ::uv_strerror( e ) : "ok" );
            // PHASE 4: finalize context destruction
            ::lws_context_destroy2( ctx_ );
        }
        else
#endif
        {
            ::lws_libuv_run( ctx_, 0 );
        bail:
            auto ctx2destroy = ctx_;
            ctx_ = nullptr;
            ::lws_context_destroy( ctx2destroy );
            ::lws_context_destroy2( ctx2destroy );
        }
    }
        return;
#endif  // (defined LWS_WITH_LIBUV)
    default:
        lwsl_debug( "server_api::service() is unavailable!\n" );
        return;
    }  // switch( srvmode_ )
}

void server_api::onConnect( connection_identifier_t cid,
                            struct lws* wsi,
                            const char* strPeerClientAddressName,
                            const char* strPeerRemoteIP ) {
    lock_type lock( mtx_api() );
    connection_data* pcd = new connection_data;
    pcd->cid_ = cid;
    // pcd->sn_ = serial_numper();
    pcd->wsi_ = wsi;
    pcd->creationTime_ = time( nullptr );
    pcd->strPeerClientAddressName_ = strPeerClientAddressName;
    pcd->strPeerRemoteIP_ = tiny::tools::format(
        "%s://%s",
        ::lws_is_ssl( wsi ) ? "wss" : "ws",
        ( strPeerRemoteIP != nullptr && strPeerRemoteIP[ 0 ] != '\0' ) ? strPeerRemoteIP : "unknown" );
    impl_eraseConnection( cid );
    connections_[ cid ] = pcd;
    if( onConnect_ )
        onConnect_( cid, wsi, strPeerClientAddressName, strPeerRemoteIP );
}
void server_api::onDisconnect( connection_identifier_t cid, const std::string& strMessage ) {
    lock_type lock( mtx_api() );
    if( onDisconnect_ )
        onDisconnect_( cid, strMessage );
    impl_removeConnection( cid );
}
void server_api::onMessage( connection_identifier_t cid, const message_payload_data& data, bool isFinalFragment ) {
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return;
    server_api::connection_data* pcd = itCnFind->second;
    if( !pcd )
        return;
    message_payload_data& a = pcd->accumulator_;
    a.append( data );
    if( !isFinalFragment )
        return;
    message_payload_data final_data = std::move( a );
    a.clear();
    if( onMessage_ ) {
        onMessage_( cid, final_data );
    }
}
void server_api::onHttp( connection_identifier_t cid ) {
    lock_type lock( mtx_api() );
    if( onHttp_ )
        onHttp_( cid );
}
void server_api::onFail( connection_identifier_t cid, const std::string& strMessage ) {
    onLogMessage( e_ws_log_message_type_t::eWSLMT_error,
                  cc::error( "Error: " ) + cc::warn( strMessage ) + " on cid " +
                      cc::num0x( reinterpret_cast< intptr_t >( cid ) ) );
    lock_type lock( mtx_api() );
    if( impl_removeConnection( cid ) ) {
        if( onFail_ )
            onFail_( cid, strMessage );
    }
}
void server_api::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& strMessage ) {
    if( onLogMessage_ )
        onLogMessage_( eWSLMT, strMessage );
}

bool server_api::send( connection_identifier_t cid, const message_payload_data& data ) {
    if( !initialized_ )
        return false;
    {
        lock_type lock( mtx_api() );
        map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
        if( itCnFind == itCnEnd )
            return false;
        server_api::connection_data* pcd = itCnFind->second;
        if( !pcd )
            return false;
        // Push this onto the buffer. It will be written out when the socket is writable.
        payload_queue_t& pq = pcd->buffer_;
        pq.push_back( data );
    }
    do_writable_callbacks_all_protocol();
    return true;
}
//			size_t server_api::broadcast( const message_payload_data & data ) { // ugly simple, we do not need it)
//				if( ! initialized_ )
//					return 0;
//				lock_type lock( mtx_api() );
//				map_connections_t::iterator itCnWalk = connections_.begin(), itCnEnd = connections_.end();
//				size_t cntSent = 0;
//				for( ; itCnWalk != itCnEnd; ++ itCnWalk ) {
//					if( send( itCnWalk->first, data ) )
//						++ cntSent;
//				}
//				return cntSent;
//			}

std::string server_api::getPeerClientAddressName( connection_identifier_t cid ) {  // notice: used as "origin"
    if( !initialized_ )
        return "";
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return "";
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd )
        return pcd->strPeerClientAddressName_;
    return "";
}
std::string server_api::getPeerRemoteIP( connection_identifier_t cid ) {  // notice: no port is returned
    if( !initialized_ )
        return "";
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return "";
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd )
        return pcd->strPeerRemoteIP_;
    return "";
}
peer_ptr_t server_api::getPeer( connection_identifier_t cid ) {
    if( !initialized_ )
        return nullptr;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return nullptr;
    server_api::connection_data* pcd = itCnFind->second;
    peer_ptr_t pPeer = nullptr;
    if( pcd )
        pPeer = pcd->getPeer();
    return pPeer;
}
peer_ptr_t server_api::detachPeer( connection_identifier_t cid ) {
    if( !initialized_ )
        return nullptr;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return nullptr;
    server_api::connection_data* pcd = itCnFind->second;
    if( !pcd )
        return nullptr;
    peer_ptr_t pPeer = pcd->getPeer();
    pcd->setPeer( nullptr );
    return pPeer;
}
bool server_api::setPeer( connection_identifier_t cid, peer_ptr_t pPeer ) {
    if( !initialized_ )
        return false;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return false;
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd ) {
        pcd->setPeer( pPeer );
        return true;
    }
    return false;
}
int64_t server_api::delayed_adjustment_pong_timeout( connection_identifier_t cid ) const {  // NLWS-specific
    if( !initialized_ )
        return -1;
    lock_type lock( mtx_api() );
    map_connections_t::const_iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return -1;
    server_api::connection_data* pcd = itCnFind->second;
    int64_t to = -1;
    if( pcd )
        to = pcd->delayed_adjustment_pong_timeout_;
    return to;
}
bool server_api::delayed_adjustment_pong_timeout( connection_identifier_t cid, int64_t to ) {  // NLWS-specific
    if( !initialized_ )
        return false;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return false;
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd ) {
        pcd->delayed_adjustment_pong_timeout_ = to;
        return true;
    }
    return false;
}

std::string server_api::getValue( connection_identifier_t cid, const std::string& strName ) {
    if( !initialized_ )
        return "";
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return "";
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd )
        return pcd->keyValueMap_[ strName ];
    return "";
}
bool server_api::setValue( connection_identifier_t cid, const std::string& strName, const std::string& strValue ) {
    if( !initialized_ )
        return false;
    lock_type lock( mtx_api() );
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return false;
    server_api::connection_data* pcd = itCnFind->second;
    if( pcd ) {
        pcd->keyValueMap_[ strName ] = strValue;
        return true;
    }
    return false;
}

int server_api::getNumberOfConnections() const {
    if( !initialized_ )
        return 0;
    lock_type lock( mtx_api() );
    return connections_.size();
}

void server_api::run( uint64_t timeout ) {
    if( !initialized_ )
        return;
    while( true ) {
        wait( timeout );
    }
}
void server_api::wait( uint64_t timeout ) {
    if( !initialized_ )
        return;
    if(::lws_service( ctx_, timeout ) < 0 )
        throw std::runtime_error( "failure while polling for socket activity" );
}

bool server_api::impl_eraseConnection( connection_identifier_t cid ) {
    map_connections_t::iterator itCnFind = connections_.find( cid ), itCnEnd = connections_.end();
    if( itCnFind == itCnEnd )
        return false;
    server_api::connection_data* pcd = itCnFind->second;
    connections_.erase( itCnFind );
    if( pcd )
        delete pcd;
    return true;
}
bool server_api::impl_removeConnection( connection_identifier_t cid ) {
    if( !initialized_ )
        return false;
    lock_type lock( mtx_api() );
    return impl_eraseConnection( cid );
}

server_api::map_ctx_2_inst_t server_api::g_ctx_2_inst;
server_api::map_inst_2_ctx_t server_api::g_inst_2_ctx;
server_api::mutex_type server_api::g_ctx_reg_mtx;
void server_api::stat_reg( server_api* api ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( api );
    assert( api->ctx_ );
    g_ctx_2_inst[ api->ctx_ ] = api;
    g_inst_2_ctx[ api ] = api->ctx_;
}
void server_api::stat_unreg( server_api* api ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( api );
    if( !api->ctx_ )
        return;
    g_ctx_2_inst.erase( api->ctx_ );
    g_inst_2_ctx.erase( api );
}
server_api* server_api::stat_get( void* ctx ) {
    lock_type lock( g_ctx_reg_mtx );
    assert( ctx );
    map_ctx_2_inst_t::iterator itFind = g_ctx_2_inst.find( ctx ), itEnd = g_ctx_2_inst.end();
    if( itFind == itEnd ) {
        //					assert( false );
        return nullptr;
    }
    server_api* api = itFind->second;
    //				assert( api );
    //				assert( api->ctx_ );
    //				assert( api->ctx_ == ctx );
    return api;
}

server_api::mutex_type server_api::g_ptr_reg_mtx;
server_api::map_ptr_2_inst_t server_api::g_ptr_2_inst;
void server_api::stat_ptr_reg( void* p, server_api* api ) {
    lock_type lock( g_ptr_reg_mtx );
    assert( p );
    assert( api );
    g_ptr_2_inst[ p ] = api;
}
void server_api::stat_ptr_unreg( void* p ) {
    lock_type lock( g_ptr_reg_mtx );
    g_ptr_2_inst.erase( p );
}
server_api* server_api::stat_ptr2api( void* p ) {
    lock_type lock( g_ptr_reg_mtx );
    assert( p );
    map_ptr_2_inst_t::iterator itFind = g_ptr_2_inst.find( p ), itEnd = g_ptr_2_inst.end();
    if( itFind == itEnd ) {
        //					assert( false );
        return nullptr;
    }
    server_api* api = itFind->second;
    //				assert( api );
    return api;
}

void server_api::onBeforeCreateContext( server_api::onBeforeCreateContext_t handler ) {
    onBeforeCreateContext_ = std::move( handler );
}

/** modifies this->server_api::ctx_info_:
 *      - adds protocols to basic_api::vec_lws_protocols_
 *      - adds/appends mount points pointer for new protocols to ctx_info_.mounts
 *      - updates protocols number
 * */
void server_api::beforeCreateContext() {
    if( onBeforeCreateContext_ != nullptr ) {
        const lws_protocols* addProtocols = nullptr;
        unsigned length = 0;
        const lws_http_mount* httpMount = nullptr;
        // Register additional end-points here
        onBeforeCreateContext_( addProtocols, length, httpMount );

        addProtocol( addProtocols, length );
        addMountPoint( httpMount );
    }
}

void server_api::addProtocol( const lws_protocols* addProtocols, unsigned int length ) {
    if( length == 0 || !addProtocols )
        return;

    // append records to vec_lws_protocols_
    auto& vec = vec_lws_protocols_;
    auto* vecData = vec.data();
    auto prevSize = vec.size();
    // Insert before last element
    const auto& insertTo = vec.empty() ? vec.end() : vec.end() - 1;
    vec.insert( insertTo, addProtocols, addProtocols + length );

    if( prevSize == 0 ) {
        lws_protocols nullMark = {};
        vec.push_back( nullMark );
    }

    // check reallocation
    if( vecData != vec.data() ) {
        ctx_info_.protocols = vec.data();
    }
}

void server_api::addMountPoint( const lws_http_mount* httpMount ) {
    if( !httpMount )
        return;

    // append to the end of mount points linked list: ctx_info_.mount
    auto*& mount = ctx_info_.mounts;
    while( mount != nullptr ) {
        mount = mount->mount_next;
    }
    if( mount == nullptr ) {
        mount = httpMount;
    }
}
