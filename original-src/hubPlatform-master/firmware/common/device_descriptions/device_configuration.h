#pragma once
#include <string>

class DeviceConfiguration final {
public:
    DeviceConfiguration( std::uint8_t channelId,
                         std::uint8_t paramNumber,
                         std::uint8_t paramSize,
                         std::uint32_t paramValue,
                         bool setToDefault,
                         const std::string& paramDescription );

public:
    std::uint8_t getChannelId() const;
    std::uint8_t getParamNumber() const;
    std::uint8_t getParamSize() const;
    std::uint32_t getParamValue() const;
    bool isSetToDefault() const;
    std::string getParamDescription() const;

private:
    std::uint8_t m_channelId;
    std::uint8_t m_paramNumber;
    std::uint8_t m_paramSize;
    std::uint32_t m_paramValue;
    bool m_isSetToDefault;
    std::string m_paramDescription;
};
