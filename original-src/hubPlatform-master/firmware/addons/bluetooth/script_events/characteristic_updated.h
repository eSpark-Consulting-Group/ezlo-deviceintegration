#pragma once

#include "bluetooth/script_events/device_event.h"

namespace HubPlatform {

class CharacteristicUpdated : public DeviceEvent {
public:
    CharacteristicUpdated( std::string deviceAddress,
                           std::string characteristicUUID,
                           std::vector< std::uint8_t > value );

    Json toJson() const override;

private:
    std::string m_characteristicUUID;
    std::vector< std::uint8_t > m_value;
};

}  // namespace HubPlatform
