#include "zwave/script_events/barrier_signal_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>
#include <znet/converters/barrier_operator_signal_converter.hpp>

namespace HubPlatform {
namespace Zwave {

BarrierSignalValueUpdated::BarrierSignalValueUpdated( uint8_t nodeId,
                                                      uint8_t channelId,
                                                      uint8_t signalId,
                                                      uint8_t value )
: ValueEvent( Resources::ScriptEvent::Name::BarrierSignalValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_BARRIER_OPERATOR,
              channelId )
, m_signalId( signalId )
, m_value( value ) {}

Json BarrierSignalValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    addOptionalStringField( result,
                            Param::SignalName,
                            znet::barrier_operator_signal_converter::signalToString,
                            static_cast< znet::BarrierOperatorSignal >( m_signalId ) );
    result[ Param::Value ] = m_value;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
