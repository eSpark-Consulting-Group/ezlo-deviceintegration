#include "bluetooth/script_events/characteristic_updated.h"

namespace HubPlatform {

CharacteristicUpdated::CharacteristicUpdated( std::string deviceAddress,
                                              std::string characteristicUUID,
                                              std::vector< std::uint8_t > value )
: DeviceEvent( "characteristic_updated", std::move( deviceAddress ) )
, m_characteristicUUID( std::move( characteristicUUID ) )
, m_value( std::move( value ) ) {}

Json CharacteristicUpdated::toJson() const {
    auto result = DeviceEvent::toJson();
    result[ "characteristicUUID" ] = m_characteristicUUID;
    result[ "value" ] = m_value;
    return result;
}
}  // namespace HubPlatform
