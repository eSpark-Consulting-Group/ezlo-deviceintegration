#include "zigbee/script_events/door_lock_operation_event_notification_received.h"
#include "zigbee/model/enums/commands/door_lock_event_source.h"
#include "zigbee/model/enums/commands/door_lock_operation_event.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

DoorLockOperatingEventNotificationReceived::DoorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                                                        uint16_t endpointId,
                                                                                        DoorLockEventSource eventSource,
                                                                                        DoorLockOperationEvent event,
                                                                                        uint16_t userId,
                                                                                        const std::string& pinCode )
: CommandReceivedBase( Resources::ScriptEvent::Name::DoorLockOperatingEventNotificationReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::DoorLock,
                       0x20,  // Operating Event Notification ID
                       Resources::Fields::Command::Names::DoorLock::OperatingEventNotification )
, m_eventSource( eventSource )
, m_event( event )
, m_userId( userId )
, m_pinCode( pinCode ) {}

Json DoorLockOperatingEventNotificationReceived::toJson() const {
    auto result = CommandReceivedBase::toJson();
    result[ Resources::ScriptEvent::Param::EventSource ] = toString( m_eventSource );
    result[ Resources::ScriptEvent::Param::EventName ] = toString( m_event );
    result[ Resources::ScriptEvent::Param::UserId ] = m_userId;
    result[ Resources::ScriptEvent::Param::PinCode ] = m_pinCode;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
