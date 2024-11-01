#pragma once

#include "json_utils/json_wrapper.h"
#include "bus/messagesclasses.h"

class Broadcast {
public:
    static void sendToAll( const MessageSubClasses eventType, JsonObject message );
    static void sendToUis( const MessageSubClasses eventType, const JsonObject& message );
    static void sendToAllNoThrow( const MessageSubClasses eventType, JsonObject message );
    static void sendToUisNoThrow( const MessageSubClasses eventType, const JsonObject& message );

private:
    static JsonObject prepareMessages( MessageSubClasses ev, const JsonObject& message );
};
