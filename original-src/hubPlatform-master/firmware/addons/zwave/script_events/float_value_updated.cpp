#include "zwave/script_events/float_value_updated.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

FloatValueUpdated::FloatValueUpdated( uint8_t nodeId,
                                      uint8_t classId,
                                      uint8_t channelId,
                                      const char* type,
                                      const char* scale,
                                      int32_t value,
                                      uint8_t precision )
: ValueEvent( Resources::ScriptEvent::Name::FloatValueUpdated, nodeId, classId, channelId )
, m_type( type )
, m_scale( scale )
, m_value( value )
, m_precision( precision ) {}

Json FloatValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Type ] = m_type;
    result[ Param::Scale ] = m_scale;
    result[ Param::Value ] = m_value;
    result[ Param::Precision ] = m_precision;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
