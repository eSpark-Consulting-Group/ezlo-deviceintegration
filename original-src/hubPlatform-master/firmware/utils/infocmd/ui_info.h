#pragma once

#include "json_utils/json_wrapper.h"
#include "bus/messagesclasses.h"

class BaseBus;

class BusInfo {
public:
    static void send( BaseBus& bus, const JsonObject& message, MessageSubClasses commands );
};

class System : private BusInfo {
public:
    static void sendMessage( BaseBus& bus, const std::string& type, const std::string& message );
};
