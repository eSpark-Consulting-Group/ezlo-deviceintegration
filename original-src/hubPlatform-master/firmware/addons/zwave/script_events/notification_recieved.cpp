#include "zwave/script_events/notification_recieved.h"
#include "zwave/resources/zwave_resources.h"
#include <tiny/tiny_utils.h>

#include "json_utils/json_utils.h"

#include <cassert>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::ScriptEvent;

NotificationReceived::NotificationReceived( uint8_t nodeId,
                                            uint8_t channelId,
                                            uint8_t v1AlarmType,
                                            uint8_t v1AlarmLevel,
                                            znet::NotificationType notification,
                                            znet::NotificationEvent event,
                                            std::vector< std::uint8_t > value )
: ValueEvent( Name::NotificationRecieved, nodeId, ZNET_COMMAND_CLASS_NOTIFICATION, channelId )
, m_v1AlarmType( v1AlarmType )
, m_v1AlarmLevel( v1AlarmLevel )
, m_notification( notification )
, m_event( event )
, m_value( std::move( value ) ) {}

Json NotificationReceived::toJson() const {
    auto result = ValueEvent::toJson();

    const auto name = znet::getNotificationName( m_notification );
    const auto eventData = znet::getNotificationEventData( m_notification, m_event );

    assert( name );
    assert( eventData );

    result[ Param::NotificationType ] = name;
    result[ Param::EventType ] = eventData->m_name;
    result[ Param::V1AlarmType ] = m_v1AlarmType;
    result[ Param::V1AlarmLevel ] = m_v1AlarmLevel;

    auto value = eventData->m_valueHandler ?
                     eventData->m_valueHandler->getNotificationValue( m_value.data(), m_value.size() ) :
                     znet::NotificationValueOpt();

    if( value ) {
        using namespace znet::visitors;
        result[ Param::Value ] =
            visit( makeOverloads( []( uint8_t val ) { return Json( val ); },
                                  []( std::string val ) { return Json( std::move( val ) ); },
                                  []( const znet::NotificationTimeValue& val ) {
                                      return JSON( Param::Hours << val.m_hours << Param::Minutes << val.m_minutes
                                                                << Param::Seconds << val.m_seconds );
                                  } ),
                   std::move( *value ) );
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
