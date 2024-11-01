#pragma once

#include <string>
#include <utility>

namespace HubPlatform {

struct DeviceAdditionalInfo {
    std::string m_label;
    std::string m_value;

    DeviceAdditionalInfo( std::string label, std::string value )
    : m_label( std::move( label ) )
    , m_value( std::move( value ) ) {}
};

}  // namespace HubPlatform
