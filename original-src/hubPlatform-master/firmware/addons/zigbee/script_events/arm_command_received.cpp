#include "zigbee/script_events/arm_command_received.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

ArmCommandReceived::ArmCommandReceived( uint16_t nodeId,
                                        uint16_t endpointId,
                                        IasAceArmCommandMode mode,
                                        const std::string& pinCode )
: CommandReceivedBase( Resources::ScriptEvent::Name::ArmCommandReceived,
                       nodeId,
                       endpointId,
                       Resources::Fields::Cluster::Names::IASAce,
                       0x00,  // Arm command ID
                       Resources::Fields::Command::Names::IasAce::Arm )
, m_mode( mode )
, m_pinCode( pinCode ) {}

Json ArmCommandReceived::toJson() const {
    auto result = CommandReceivedBase::toJson();
    result[ Resources::ScriptEvent::Param::ArmMode ] = toString( m_mode );
    result[ Resources::ScriptEvent::Param::PinCode ] = m_pinCode;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
