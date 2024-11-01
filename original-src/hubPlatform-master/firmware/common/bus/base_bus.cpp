
#include "bus/base_bus.h"
#include "json_utils/json_utils.h"
#include "system_utils/logger.h"
#include "messagesclasses.h"

BaseBus::BaseBus( const std::string& privateName, const RxCallBack& rxCallBack )
: m_privateName( privateName )
, m_rxCallback( rxCallBack ) {}

bool BaseBus::isUiBroadcastMessage( int msgClass ) {
    return ( msgClass == HA_EXCEPTION ) || ( msgClass == UI_EVENT ) || ( msgClass == LOG_EVENT ) ||
           ( msgClass == EZLO_EVENT );
}

bool BaseBus::isBroadcastMessage( int msgClass ) {
    return ( msgClass == EVENT ) || isUiBroadcastMessage( msgClass );
}

bool BaseBus::send( const char* group, int msgClass, int msgSubClass, const std::string& msg ) {
    JsonObject json_msg;
    if( !parseJson( msg, json_msg ) ) {
        HubPlatform::SystemUtils::logError() << "Failed to parse JSON: " << msg << std::endl;
        return true;
    }

    return send( group, msgClass, msgSubClass, json_msg );
}

bool BaseBus::send( const char* group, int msgClass, int msgSubclass, const JsonObject& msg ) {
    JsonObject json_data;
    json_add_str( json_data, "msg_subclass", msgSubclassToStr( msgClass, msgSubclass ) );
    if( isUiBroadcastMessage( msgClass ) ) {
        json_add_str( json_data, "id", "ui_broadcast" );
    }
    json_data[ "result" ] = msg;

    return multicast( group, msgClass, json_data.dump() );
}

const std::string& BaseBus::getPrivateName() const {
    return m_privateName;
}

const RxCallBack& BaseBus::getRxCallback() const {
    return m_rxCallback;
}

int BaseBus::maxMessageLength() const {
    return Limits::MaxMulticastMessageLength;
}

void BaseBus::checkMessageLength( int messageLength ) {
    if( messageLength > maxMessageLength() ) {
        HubPlatform::SystemUtils::logError() << "Bus message is longer than the limit" << std::endl;
        throw MaxMessageLengthError( "Bus message is longer than the limitation: " +
                                     std::to_string( maxMessageLength() ) );
    }
}
