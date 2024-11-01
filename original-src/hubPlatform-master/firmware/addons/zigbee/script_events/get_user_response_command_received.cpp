#include "zigbee/script_events/get_user_response_command_received.h"
#include "zigbee/model/enums/commands/user_type.h"
#include "zigbee/model/enums/commands/user_status.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

GetUserResponseReceived::GetUserResponseReceived( uint16_t nodeId,
                                                  uint16_t endpointId,
                                                  uint16_t userId,
                                                  UserStatus userStatus,
                                                  UserType userType,
                                                  const std::string& pinCode )
: CommandReceivedBase( Resources::ScriptEvent::Name::GetUserResponseCommandReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::DoorLock,
                       0x06,  // Get PIN Code Response command ID
                       Resources::Fields::Command::Names::DoorLock::GetPinCodeResponse )
, m_userId( userId )
, m_userStatus( userStatus )
, m_userType( userType )
, m_pinCode( pinCode ) {}

Json GetUserResponseReceived::toJson() const {
    auto result = CommandReceivedBase::toJson();
    result[ Resources::ScriptEvent::Param::UserId ] = m_userId;
    result[ Resources::ScriptEvent::Param::UserStatus ] = toString( m_userStatus );
    result[ Resources::ScriptEvent::Param::UserType ] = toString( m_userType );
    result[ Resources::ScriptEvent::Param::PinCode ] = m_pinCode;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
