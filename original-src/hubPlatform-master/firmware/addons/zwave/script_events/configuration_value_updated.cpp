#include "zwave/script_events/configuration_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>
#include <znet/converters/signed_configuration_parameter_converter.hpp>

namespace HubPlatform {
namespace Zwave {

ConfigurationValueUpdated::ConfigurationValueUpdated( uint8_t nodeId,
                                                      uint8_t channelId,
                                                      uint16_t parameterNumber,
                                                      uint8_t parameterSize,
                                                      uint32_t value,
                                                      const znet::ConfigurationFormat* format )
: ValueEvent( Resources::ScriptEvent::Name::ConfigurationValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_CONFIGURATION,
              channelId )
, m_parameterNumber( parameterNumber )
, m_parameterSize( parameterSize )
, m_format( format )
, m_value( value ) {}

Json ConfigurationValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;
    using namespace znet::signed_configuration_parameter_converter;

    auto result = ValueEvent::toJson();
    result[ Param::ParameterNum ] = m_parameterNumber;
    result[ Param::ParameterSize ] = m_parameterSize;
    result[ Param::Value ] = m_format ? toUiValue( m_value, m_parameterSize, *m_format ) : m_value;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
