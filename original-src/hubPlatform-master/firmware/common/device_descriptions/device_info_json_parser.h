#pragma once

#include "device_descriptions_container.h"
#include "device_descriptions/common_types.h"

#include "json_utils/json_utils.h"
#include "base_utils/non_copyable.h"

#include <memory>

class DeviceInfoJsonParser final : public common::NonCopyable {
public:
    DeviceDescriptionsContainer load( const FilenameType& fileName );

private:
    ManufacturersNames parseManufacturers( const JsonObject& rootObject );
    DeviceTypesList parseTypes( const JsonObject& rootObject );
    DeviceDescriptions parseDescriptions( const JsonObject& rootObject );
    DeviceConfigurations parseConfigurationsOptional( const JsonObject& rootObject );
    ItemSubTypesList parseItemSubTypes( const JsonObject& rootObject );
    DeviceSubTypesList parseDeviceSubTypes( const JsonObject& rootObject );
    DevicesShowAsRuleExcludedItems parseDevicesShowAsRuleExcludedItems( const JsonObject& rootObject );

private:
    DeviceInfoCommonList parseIdNameNodes( const std::string& nodeName, const JsonObject& rootObject );

private:
    template < typename T >
    void json_get_checked( const Json& vals, const std::string& key, T& res );

    template < typename T >
    void json_get_optional( const Json& vals, const std::string& key, T& res, T&& defaultValue );

    template < typename T >
    T json_get_optional( const Json& vals, const std::string& key, T&& defaultValue );
};
