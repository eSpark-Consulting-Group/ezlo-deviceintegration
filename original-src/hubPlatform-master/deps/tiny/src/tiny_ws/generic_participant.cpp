generic_participant::generic_participant() {}

generic_participant::~generic_participant() {}

generic_sender::generic_sender() {}
generic_sender::~generic_sender() {}
bool generic_sender::sendMessage( const std::string& msg ) {  // send as text
    return sendMessageText( msg );
}
bool generic_sender::sendMessage( const nlohmann::json& msg ) {
    return sendMessageText( msg.dump() );
}

e_ws_logging_level_t g_eWSLL = e_ws_logging_level_t::eWSLL_none;

e_ws_logging_level_t str2wsll( const std::string& s ) {
    std::string v = tiny::tools::to_lower( s );
    if( v.empty() )
        return e_ws_logging_level_t::eWSLL_none;
    char c = v[ 0 ];
    switch( c ) {
    case 'b':
        return e_ws_logging_level_t::eWSLL_basic;
    case 'd':
        return e_ws_logging_level_t::eWSLL_detailed;
    default:
        return e_ws_logging_level_t::eWSLL_none;
    }  // switch( c )
}
std::string wsll2str( e_ws_logging_level_t eWSLL ) {
    switch( eWSLL ) {
    case e_ws_logging_level_t::eWSLL_basic:
        return "basic";
    case e_ws_logging_level_t::eWSLL_detailed:
        return "detailed";
    default:
        return "none";
    };  // switch( eWSLL )
}
