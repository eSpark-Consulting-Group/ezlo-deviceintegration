#pragma once

#include "zwave/script_events/value_event.h"
#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class DoorLockConfigurationUpdated : public ValueEvent {
public:
    using Configuration = znet_door_lock_configuration_t;
    using ExtendedConfiguration = znet_door_lock_extended_configuration_t;
    using ExtendedConfigurationOpt = std::pair< ExtendedConfiguration, bool >;

    DoorLockConfigurationUpdated( const uint8_t nodeId,
                                  const uint8_t channelId,
                                  const Configuration& configuration,
                                  const ExtendedConfigurationOpt& extendedConfigurationOpt );

    Json toJson() const final;

private:
    Configuration m_configuration;
    ExtendedConfigurationOpt m_extendedConfigurationOpt;
};

}  // namespace Zwave
}  // namespace HubPlatform
