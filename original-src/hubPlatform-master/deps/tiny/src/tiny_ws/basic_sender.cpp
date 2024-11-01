basic_sender::basic_sender() {}
basic_sender::~basic_sender() {}
nlohmann::json basic_sender::toJSON( bool /*bSkipEmptyStats = true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "sender";
    return jo;
}
