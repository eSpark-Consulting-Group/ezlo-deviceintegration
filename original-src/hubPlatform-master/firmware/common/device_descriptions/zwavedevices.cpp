#include <string>
#include "device_descriptions_container.h"
#include "device_info_json_parser.h"
#include "zwavedevices.h"
#include "device_info.h"

static const FilenameType DeviceInfoFilePath = "/opt/firmware/etc/zwave_devices_info.json";

static const std::string DeviceTypeUnknown = "UNKNOWN";
static const std::string ItemSubTypeUnknown = "";
static const std::string DeviceSubTypeUnknown = "device";
static const std::string EmptyStringAsUndefined = "";
static const DeviceConfigurations EmptyDefaultConfigurations;

DeviceInfoManager::DeviceInfoManager() {
    DeviceInfoJsonParser deviceInfoParser;
    m_container = deviceInfoParser.load( DeviceInfoFilePath );
}

bool DeviceInfoManager::get( const DeviceId& deviceId,
                             std::string& deviceManufacturer,
                             std::string& deviceName,
                             std::string& deviceModel,
                             int& deviceType ) {
    if( !m_container.empty() ) {
        const auto& manufacturerName = m_container.findDeviceManufacturer( deviceId.manufacturerId() );
        const auto& deviceInfo = m_container.findDeviceInfo( deviceId );
        if( manufacturerName.empty() || ( deviceInfo == nullptr ) ) {
            return false;
        }

        deviceManufacturer = manufacturerName;
        deviceName = deviceInfo->getName();
        deviceModel = deviceInfo->getModel();
        deviceType = deviceInfo->getType();

        return true;
    }

    return false;
}

const std::string& DeviceInfoManager::getTypeName( int deviceTypeEnumId ) noexcept {
    if( !m_container.empty() ) {
        const auto& devType = m_container.findDeviceType( deviceTypeEnumId );
        return devType.empty() ? DeviceTypeUnknown : devType;
    }
    return DeviceTypeUnknown;
}

bool DeviceInfoManager::hasExtraFields( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        auto descriptions = m_container.getDeviceDescriptioins();
        auto test = m_container.findDeviceShowAsRuleExcludedItem(
            std::to_string( deviceId.manufacturerProductId() ) + std::to_string( deviceId.manufacturerId() ) +
            std::to_string( deviceId.manufacturerProductType() ) );

        if( descriptions.end() != descriptions.find( deviceId ) ) {
            return descriptions[ deviceId ]->hasExtraFields();
        }
    }

    return false;
}

std::vector< std::string > DeviceInfoManager::getDevicesShowAsRuleExcludedItems( const DeviceId& deviceId ) noexcept {
    std::vector< std::string > excluded_items;

    if( !m_container.empty() ) {
        if( deviceId.manufacturerId() == -1 && deviceId.manufacturerProductId() == -1 &&
            deviceId.manufacturerProductType() == -1 ) {
            excluded_items = m_container.findDeviceShowAsRuleExcludedItem( "default" );
        }
        else {
            excluded_items = m_container.findDeviceShowAsRuleExcludedItem(
                std::to_string( deviceId.manufacturerProductId() ) + std::to_string( deviceId.manufacturerId() ) +
                std::to_string( deviceId.manufacturerProductType() ) );
        }
    }

    return excluded_items;
}

bool DeviceInfoManager::hasGetter( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        auto descriptions = m_container.getDeviceDescriptioins();

        if( descriptions.end() != descriptions.find( deviceId ) ) {
            return descriptions[ deviceId ]->hasGetter();
        }
    }
    return false;
}

bool DeviceInfoManager::hasSetter( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        auto descriptions = m_container.getDeviceDescriptioins();

        if( descriptions.end() != descriptions.find( deviceId ) ) {
            return descriptions[ deviceId ]->hasSetter();
        }
    }
    return false;
}

const std::string& DeviceInfoManager::getItemSubtypeName( const DeviceId& deviceId, int instanceId ) noexcept {
    if( !m_container.empty() ) {
        const auto& deviceInfo = m_container.findDeviceInfo( deviceId );
        if( deviceInfo != nullptr ) {
            const auto& itemSubTypes = deviceInfo->getItemsSubTypes();
            auto iter = itemSubTypes.find( instanceId );
            if( iter != itemSubTypes.end() ) {
                auto itemUnderlyingType = iter->second;
                const auto& subTypeName = m_container.findDeviceItemSubType( itemUnderlyingType );
                return subTypeName.empty() ? ItemSubTypeUnknown : subTypeName;
            }
        }
    }

    return ItemSubTypeUnknown;
}

const std::string& DeviceInfoManager::getDeviceSubtypeName( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        const auto& deviceInfo = m_container.findDeviceInfo( deviceId );
        if( deviceInfo != nullptr ) {
            auto deviceSubType = deviceInfo->getSubType();
            const auto& deviceSubTypeName = m_container.findDeviceSubType( deviceSubType );
            return deviceSubTypeName.empty() ? DeviceSubTypeUnknown : deviceSubTypeName;
        }
    }

    return DeviceSubTypeUnknown;
}

const std::string& DeviceInfoManager::getMainItemName( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        const auto& deviceInfo = m_container.findDeviceInfo( deviceId );
        if( deviceInfo != nullptr ) {
            const auto& mainItemName = deviceInfo->getMainItemName();
            return mainItemName.empty() ? EmptyStringAsUndefined : mainItemName;
        }
    }
    return EmptyStringAsUndefined;
}

const DeviceConfigurations& DeviceInfoManager::getConfigurations( const DeviceId& deviceId ) noexcept {
    if( !m_container.empty() ) {
        const auto& deviceInfo = m_container.findDeviceInfo( deviceId );
        if( deviceInfo ) {
            const auto& deviceConfiguration = deviceInfo->getConfigurations();
            return deviceConfiguration.empty() ? EmptyDefaultConfigurations : deviceConfiguration;
        }
    }

    return EmptyDefaultConfigurations;
}

DeviceInfoManager& DeviceInfoManager::instance() {
    static DeviceInfoManager instance;

    return instance;
}
