#include "zigbee/script_events/door_lock_programming_event_notification_received.h"
#include "zigbee/model/enums/commands/door_lock_event_source.h"
#include "zigbee/model/enums/commands/door_lock_programming_event.h"
#include "zigbee/model/enums/commands/user_type.h"
#include "zigbee/model/enums/commands/user_status.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

DoorLockProgrammingEventNotificationReceived::DoorLockProgrammingEventNotificationReceived(
    uint16_t nodeId,
    uint16_t endpointId,
    DoorLockEventSource eventSource,
    DoorLockProgrammingEvent event,
    uint16_t userId,
    const std::string& pinCode,
    UserType userType,
    UserStatus userStatus )
: CommandReceivedBase( Resources::ScriptEvent::Name::DoorLockProgrammingEventNotificationReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::DoorLock,
                       0x21,  // Programming Event Notification ID
                       Resources::Fields::Command::Names::DoorLock::ProgrammingEventNotification )
, m_eventSource( eventSource )
, m_event( event )
, m_userId( userId )
, m_pinCode( pinCode )
, m_userType( userType )
, m_userStatus( userStatus ) {}

Json DoorLockProgrammingEventNotificationReceived::toJson() const {
    auto result = CommandReceivedBase::toJson();
    result[ Resources::ScriptEvent::Param::EventSource ] = toString( m_eventSource );
    result[ Resources::ScriptEvent::Param::EventName ] = toString( m_event );
    result[ Resources::ScriptEvent::Param::UserId ] = m_userId;
    result[ Resources::ScriptEvent::Param::PinCode ] = m_pinCode;
    result[ Resources::ScriptEvent::Param::UserType ] = toString( m_userType );
    result[ Resources::ScriptEvent::Param::UserStatus ] = toString( m_userStatus );

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
