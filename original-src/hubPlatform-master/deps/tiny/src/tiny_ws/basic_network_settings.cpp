basic_network_settings::basic_network_settings()
: interval_ping_( 20 /*same as NMA*/ )  // seconds, ping-pong interval, 0 means not use
// Disable next 3 timeouts. We do not handle them properly:
// - need to manually send pings to each peer (client or server)
// - need reset timeouts for each peer on any message received
, timeout_pong_( 0 )             // seconds, default value in wspp is 5000, 0 means not use
, timeout_handshake_open_( 0 )   // seconds, default value in wspp is 5000, 0 means not use
, timeout_handshake_close_( 0 )  // seconds, default value in wspp is 5000, 0 means not use
//#if(defined __HAVE_TINY_WS_BACKEND_NLWS__)
//			, max_message_size_( 0 ) // bytes, 0 is unlimited
//			, max_body_size_( 0 ) // bytes, 0 is unlimited
//#else /// (defined __HAVE_TINY_WS_BACKEND_NLWS__)
, max_message_size_( 32 * 1000 * 1000 )  // bytes, default value in wspp is 32000000
, max_body_size_( 32 * 1000 * 1000 )     // bytes, default value in wspp is 32000000
//#endif /// else from (defined __HAVE_TINY_WS_BACKEND_NLWS__)
, timeout_restart_on_close_( 3 )  // seconds
, timeout_restart_on_fail_( 2 )   // seconds
, log_ws_rx_tx_( true )

, server_disable_ipv6_( false )         // use LWS_SERVER_OPTION_DISABLE_IPV6 option
, server_validate_utf8_( false )        // use LWS_SERVER_OPTION_VALIDATE_UTF8 option
, server_skip_canonical_name_( false )  // use LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME option
, ssl_perform_global_init_( true )      // both client and server, use LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT option
, ssl_server_require_valid_certificate_( false )  // use LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT option
, ssl_server_allow_non_ssl_on_ssl_port_( false )  // use LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT option
, ssl_server_accept_connections_without_valid_cert_( true )  // use LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED option
, ssl_server_redirect_http_2_https_( true )                  // use LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS option
, ssl_client_allow_self_signed_( false )                     // use LCCSCF_ALLOW_SELFSIGNED option
, ssl_client_skip_host_name_check_( true )                   // use LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK option
, ssl_client_allow_expired_( false )                         // use LCCSCF_ALLOW_EXPIRED option
, cntMaxPortionBytesToSend_( 0 ) {}
basic_network_settings& basic_network_settings::default_instance() {
    static basic_network_settings g_inst;
    return g_inst;
}
void basic_network_settings::bns_assign_from_default_instance() {
    ( *this ) = default_instance();
}
