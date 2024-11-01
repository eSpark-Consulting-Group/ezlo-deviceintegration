#include "device_descriptions/device_configuration.h"

DeviceConfiguration::DeviceConfiguration( std::uint8_t channelId,
                                          std::uint8_t paramNumber,
                                          std::uint8_t paramSize,
                                          std::uint32_t paramValue,
                                          bool setToDefault,
                                          const std::string& paramDescription )
: m_channelId( channelId )
, m_paramNumber( paramNumber )
, m_paramSize( paramSize )
, m_paramValue( paramValue )
, m_isSetToDefault( setToDefault )
, m_paramDescription( paramDescription ) {}

std::uint8_t DeviceConfiguration::getChannelId() const {
    return m_channelId;
}

std::uint8_t DeviceConfiguration::getParamNumber() const {
    return m_paramNumber;
}

std::uint8_t DeviceConfiguration::getParamSize() const {
    return m_paramSize;
}

uint32_t DeviceConfiguration::getParamValue() const {
    return m_paramValue;
}

bool DeviceConfiguration::isSetToDefault() const {
    return m_isSetToDefault;
}

std::string DeviceConfiguration::getParamDescription() const {
    return m_paramDescription;
}
