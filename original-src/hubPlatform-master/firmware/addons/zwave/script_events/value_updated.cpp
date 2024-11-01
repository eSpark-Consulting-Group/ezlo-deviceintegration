#include "zwave/script_events/value_updated.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ValueUpdated::ValueUpdated( uint8_t nodeId,
                            uint8_t classId,
                            uint8_t channelId,
                            uint8_t currentValue,
                            znet::utils::optional< uint8_t > targetValue,
                            znet::utils::optional< std::chrono::seconds > targetDuration )
: ValueEvent( Resources::ScriptEvent::Name::ValueUpdated, nodeId, classId, channelId )
, m_currentValue( currentValue )
, m_targetValue( targetValue )
, m_targetDuration( targetDuration ) {}

Json ValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();

    result[ Param::Value ] = m_currentValue;

    if( m_targetValue )
        result[ Param::TargetValue ] = *m_targetValue;

    if( m_targetDuration )
        result[ Param::TargetDuration ] =
            std::chrono::duration_cast< std::chrono::milliseconds >( *m_targetDuration ).count();

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
