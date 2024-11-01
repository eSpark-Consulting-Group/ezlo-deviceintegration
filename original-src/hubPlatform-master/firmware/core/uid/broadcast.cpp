#include "uid/broadcast.h"
#include "uid/ui_broadcaster.h"
#include "uid/ezlo.h"

#include "database_client/dbid.h"

#include <mutex>

void Broadcast::sendToAll( const MessageSubClasses eventType, JsonObject message ) {
    static std::mutex m;
    std::lock_guard< std::mutex > lock( m );
    json_remove( message, "id" );
    json_remove( message, "sender" );
    json_add_str( message, "serial", Ezlo::serial() );
    JsonObject msg = prepareMessages( eventType, message );
    UidBroadcasterPtr uidBroadcaster = global_uid_broadcaster();
    if( uidBroadcaster )
        uidBroadcaster->postMessage( msg.dump() );
}

void Broadcast::sendToUis( const MessageSubClasses eventType, const JsonObject& message ) {
    UidBroadcasterPtr uidBroadcaster = global_uid_broadcaster();
    if( uidBroadcaster )
        uidBroadcaster->postMessage( prepareMessages( eventType, message ).dump() );
}

void Broadcast::sendToAllNoThrow( const MessageSubClasses eventType, JsonObject message ) {
    try {
        sendToAll( eventType, message );
    }
    catch( ... ) {
    }
}
void Broadcast::sendToUisNoThrow( const MessageSubClasses eventType, const JsonObject& message ) {
    try {
        sendToUis( eventType, message );
    }
    catch( ... ) {
    }
}

JsonObject Broadcast::prepareMessages( MessageSubClasses ev, const JsonObject& message ) {
    JsonObject json_data;
    json_add_str( json_data, "msg_subclass", msgSubclassToStr( UI_EVENT, ev ) );
    json_add_str( json_data, "id", "ui_broadcast" );
    json_add_str( json_data, "msg_id", db::DbId::genOID() );
    json_data[ "result" ] = message;

    return json_data;
}
