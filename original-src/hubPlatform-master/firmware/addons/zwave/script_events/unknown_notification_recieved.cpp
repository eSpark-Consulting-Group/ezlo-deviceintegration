#include "zwave/script_events/unknown_notification_recieved.h"
#include "zwave/resources/zwave_resources.h"
#include <tiny/tiny_utils.h>

#include <cassert>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::ScriptEvent;

UnknownNotificationReceived::UnknownNotificationReceived( uint8_t nodeId,
                                                          uint8_t channelId,
                                                          uint8_t v1AlarmType,
                                                          uint8_t v1AlarmLevel,
                                                          znet::NotificationType notification,
                                                          znet::NotificationEvent event,
                                                          std::vector< std::uint8_t > value )
: ValueEvent( Name::UnknownNotificationRecieved, nodeId, ZNET_COMMAND_CLASS_NOTIFICATION, channelId )
, m_v1AlarmType( v1AlarmType )
, m_v1AlarmLevel( v1AlarmLevel )
, m_notification( notification )
, m_event( event )
, m_value( std::move( value ) )

{}

Json UnknownNotificationReceived::toJson() const {
    auto result = ValueEvent::toJson();

    const auto name = znet::getNotificationName( m_notification );

    result[ Param::NotificationType ] = name ? name : Param::Notification::UndefinedNotificationType;
    result[ Param::NotificationTypeRawValue ] = static_cast< uint8_t >( m_notification );
    result[ Param::EventType ] = m_event;
    result[ Param::V1AlarmType ] = m_v1AlarmType;
    result[ Param::V1AlarmLevel ] = m_v1AlarmLevel;

    if( !m_value.empty() ) {
        result[ Param::RawBase64Value ] = tiny::tools::base64::encode( m_value.data(), m_value.size() );
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
