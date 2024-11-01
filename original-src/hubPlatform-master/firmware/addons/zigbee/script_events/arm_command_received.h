#pragma once

#include "zigbee/script_events/command_received_base.h"
#include "zigbee/model/enums/commands/ias_ace_arm_command_mode.h"

namespace HubPlatform {
namespace Zigbee {

class ArmCommandReceived : public CommandReceivedBase {
public:
    explicit ArmCommandReceived( uint16_t nodeId,
                                 uint16_t endpointId,
                                 IasAceArmCommandMode mode,
                                 const std::string& pinCode );

    Json toJson() const override;

private:
    IasAceArmCommandMode m_mode;
    std::string m_pinCode;
};

}  // namespace Zigbee
}  // namespace HubPlatform
