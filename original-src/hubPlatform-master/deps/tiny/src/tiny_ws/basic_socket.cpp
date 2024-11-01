basic_socket::basic_socket() {
    bns_assign_from_default_instance();
}
basic_socket::~basic_socket() {}
nlohmann::json basic_socket::toJSON( bool /*bSkipEmptyStats = true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "socket";
    return jo;
}
void basic_socket::close() {}

void basic_socket::cancel() {}
void basic_socket::pause_reading() {}

void basic_socket::onOpen( hdl_t hdl ) {
    if( onOpen_ )
        onOpen_( *this, hdl );
}
void basic_socket::onClose( hdl_t hdl,
                            const std::string& reason,
                            int local_close_code,
                            const std::string& local_close_code_as_str ) {
    if( onClose_ )
        onClose_( *this, hdl, reason, local_close_code, local_close_code_as_str );
}
void basic_socket::onFail( hdl_t hdl ) {
    if( onFail_ )
        onFail_( *this, hdl );
}
void basic_socket::onMessage( hdl_t hdl, opcv eOpCode, const std::string& msg ) {
    if( onMessage_ )
        onMessage_( *this, hdl, eOpCode, msg );
}
// void basic_socket::onStreamSocketInit( int native_fd ) {
//	std::stringstream ss;
//	tiny::network::basic_stream_socket_init( native_fd, ss );
//	std::string msg = ss.str();
//	if( ! msg.empty() )
//		onLogMessage( e_ws_log_message_type_t::eWSLMT_error, msg );
//}
void basic_socket::onLogMessage( e_ws_log_message_type_t eWSLMT, const std::string& msg ) {
    if( !msg.empty() ) {
        if( bLogToStdCerr_ )
            std::cerr << msg;
        if( onLogMessage_ )
            onLogMessage_( *this, eWSLMT, msg );
    }
}
