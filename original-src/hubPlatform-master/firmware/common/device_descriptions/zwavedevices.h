#pragma once

#include "device_descriptions/device_id.h"
#include "device_descriptions/device_descriptions_container.h"
#include "device_descriptions/zwave_definitions.h"
#include "device_descriptions/device_configuration.h"

#include "base_utils/non_copyable.h"

class DeviceInfoManager final : public common::NonCopyable {
private:
    DeviceInfoManager();

public:
    bool get( const DeviceId& deviceId,
              std::string& deviceManufacturer,
              std::string& deviceName,
              std::string& deviceModel,
              int& deviceType );

    const std::string& getTypeName( int deviceTypeEnumId ) noexcept;
    const std::string& getItemSubtypeName( const DeviceId& deviceId, int instanceId ) noexcept;
    const std::string& getDeviceSubtypeName( const DeviceId& deviceId ) noexcept;
    const std::string& getMainItemName( const DeviceId& deviceId ) noexcept;
    const DeviceConfigurations& getConfigurations( const DeviceId& deviceId ) noexcept;

    bool hasExtraFields( const DeviceId& deviceId ) noexcept;
    std::vector< std::string > getDevicesShowAsRuleExcludedItems( const DeviceId& deviceId ) noexcept;

    bool hasGetter( const DeviceId& deviceId ) noexcept;
    bool hasSetter( const DeviceId& deviceId ) noexcept;

    static DeviceInfoManager& instance();

private:
    DeviceDescriptionsContainer m_container;
};
