#pragma once

#include "json_utils/json_fwd.h"
#include "provision_data_utils/provision_data_manager.h"

#include <string>
#include <set>

namespace HubPlatform {
namespace CoreD {

class DeviceManagerException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class NestedParentDevicesNotAllowedException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class CoreService;
class DbModifier;

class DeviceManager final {
public:
    static constexpr size_t NameMaxLength = 30;
    DeviceManager(
        CoreD::CoreService& hostService,
        const ProvisionDataUtils::ProvisionData& provisionData = ProvisionDataUtils::getManager().getProvisionData() );

    /* json fields:
     *   "id": "a unique id" */
    using GatewayID = JsonObject;

    /* json fields:
     *   "id": "a unique id" */
    using DeviceID = JsonObject;

    /* json fields:
     *   "id": "a unique id" */
    using ItemID = JsonObject;

    /* json fields:
     *   "id": "a unique id" */
    using DeviceSettingID = JsonObject;

    /* json fields:
     *   "id": "plugin_id.setting_name" */
    using GatewaySettingId = JsonObject;

    GatewayID ensureGateway( const JsonObject& gateway );
    GatewaySettingId addGatewaySetting( const JsonObject& setting );
    DeviceID addDevice( const JsonObject& device );
    DeviceID renameDevice( const JsonObject& deviceName );
    DeviceID placeDevice( const std::string& deviceId,
                          const std::string& roomId,
                          const std::set< std::string >& separateChildDevice );
    void unplaceDevice( const std::string& deviceId );
    ItemID addItem( const JsonObject& item );
    DeviceSettingID addDeviceSetting( const JsonObject& joSetting );
    DeviceID setDeviceArmed( const JsonObject& deviceArmedData );
    DeviceID setDeviceProtectConfig( const JsonObject& deviceProtectConfigData );

    GatewayID modifyGateway( const std::string& gatewayId, const JsonObject& gateway );
    DeviceID modifyDevice( const std::string& deviceId, const JsonObject& device );
    ItemID modifyItem( const std::string& itemId, const JsonObject& item );
    DeviceSettingID modifySetting( const std::string& settingId, const JsonObject& setting );

    JsonObject fetchDevice( const std::string& deviceId );
    JsonObject fetchItem( const std::string& itemId );

private:
    std::string generateUniqueID();

    void checkValueIndentItem( const JsonObject& item ) const;

private:
    CoreD::CoreService& m_coreService;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
};

}  // namespace CoreD
}  // namespace HubPlatform
