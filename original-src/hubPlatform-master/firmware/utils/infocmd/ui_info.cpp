#include "ui_info.h"
#include "bus/spread_bus.h"
#include "json_utils/json_utils.h"

void BusInfo::send( BaseBus& bus, const JsonObject& message, MessageSubClasses commands ) {
    std::string str = json_write( message );
    bus.send( "broadcast_group", SYSTEM_INFO, commands, str );
}

void System::sendMessage( BaseBus& bus, const std::string& type, const std::string& message ) {
    JsonObject msg;
    json_add_str( msg, "type", type );
    json_add_str( msg, "message", message );
    BusInfo::send( bus, msg, SYSTEM_BROADCAST );
}
