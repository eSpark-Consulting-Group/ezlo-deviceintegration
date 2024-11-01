#include "zwave/script_events/rgb_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/converters/color_switch_converter.hpp>

namespace HubPlatform {
namespace Zwave {

RgbValueUpdated::RgbValueUpdated( uint8_t nodeId,
                                  uint8_t classId,
                                  uint8_t channelId,
                                  znet::ColorComponentInfos components )
: ValueEvent( Resources::ScriptEvent::Name::RgbValueUpdated, nodeId, classId, channelId )
, m_components( std::move( components ) ) {}

Json RgbValueUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    auto& joColors = result[ Resources::ScriptEvent::Param::Colors ];
    auto& joTargetColors = result[ Resources::ScriptEvent::Param::TargetColors ];
    auto& joTargetDurations = result[ Resources::ScriptEvent::Param::TargetDurations ];
    for( const auto& component : m_components ) {
        if( const char* idStr = znet::color_switch_converter::componentIdToStr( component.id ) ) {
            joColors[ idStr ] = component.value;
            if( component.targetValue ) {
                joTargetColors[ idStr ] = *component.targetValue;
            }

            if( component.targetDuration ) {
                joTargetDurations[ idStr ] = component.targetDuration->count();
            }
        }
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
