basic_participant::basic_participant() {}
basic_participant::~basic_participant() {}
std::string basic_participant::stat_backend_name() {
    return "nlws";
}
nlohmann::json basic_participant::toJSON( bool /*bSkipEmptyStats = true*/ ) const {
    nlohmann::json jo = nlohmann::json::object();
    jo[ "type" ] = "participant";
    return jo;
}
