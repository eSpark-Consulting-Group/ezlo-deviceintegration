#include "migration_tool/migration/v1_0/migration.h"

#include "migration_tool/utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_0 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    const auto isRenamed = Utils::renameCollections(
        m_dbClient,
        { // Firmware persistent
          { "SYS.backup", "db.firmware.backupd.backup" },
          { "temp.backup", "temp.db.firmware.backupd.backup" },
          { "ACL.permission", "db.firmware.common.aclPermission" },
          { "DB.versions", "db.firmware.common.dataVersions" },
          { "SYS.flags", "db.firmware.common.flags" },
          { "HA.devices", "db.firmware.cored.devices" },
          { "HA.gateways", "db.firmware.cored.gateways" },
          { "HA.HouseModes", "db.firmware.cored.houseModes" },
          { "UI.pages", "db.firmware.cored.rooms" },
          { "UI.rooms", "db.firmware.cored.rooms" },
          { "HA.rooms", "db.firmware.cored.rooms" },
          { "UI.PageOrder", "db.firmware.cored.roomsOrder" },
          { "UI.RoomsOrder", "db.firmware.cored.roomsOrder" },
          { "HA.RoomsOrder", "db.firmware.cored.roomsOrder" },
          { "SYS.settings", "db.firmware.cored.settings" },
          { "HUB.settings", "db.firmware.cored.settings" },
          { "LOG.devicelog", "db.firmware.cored.devicelog" },
          { "Network.staticInfo", "db.firmware.networkd.staticInfo" },
          { "Network.wifiSettings", "db.firmware.networkd.wifiSettings" },
          { "Rules.rules", "db.firmware.scenes_manager.rules" },
          { "Firmware.property", "db.firmware.uid.firmwareProperty" },
          { "HUB.cache", "db.firmware.uid.cache" },
          { "HA.cache", "db.firmware.uid.cache" },
          { "HA.dataitems", "db.firmware.uid.dataitems" },
          { "HA.favorites", "db.firmware.uid.favorites" },
          { "HA.items", "db.firmware.uid.items" },
          { "HA.subDevicesFavorites", "db.firmware.uid.subDevicesFavorites" },
          { "SYS.owner", "db.firmware.uid.owner" },
          { "UI.Preferences", "db.firmware.uid.preferences" },
          { "UI.RuleOrder", "db.firmware.uid.ruleOrder" },

          // Firmware temporary
          { "temp.HA.devices", "temp.db.firmware.cored.devices" },
          { "temp.HA.HouseModes", "temp.db.firmware.cored.houseModes" },
          { "temp.HA.items", "temp.db.firmware.uid.items" },
          { "temp.NM.interfaces", "db.firmware.networkd.interfaces" },
          { "temp.EM.Plugins", "temp.db.firmware.extensions_manager.plugins" },
          { "temp.UM.UnavailablePlugins", "temp.db.firmware.update_manager.unavailablePlugins" },

          // Addons
          { "Bluetooth.devices", "db.addons.bluetooth.devices" },
          { "HA.storages", "db.addons.cameras.storages" },
          { "Cameras.storages", "db.addons.cameras.storages" },
          { "addon.plugins", "db.addons.luup.plugins" },
          { "Zigbee.devices", "db.addons.zigbee.devices" },
          { "Zwave.devices", "db.addons.zwave.devices" },

          // Plugins
          { "bluetooth", "db.plugins.bluetooth.storage" },
          { "cameras", "db.plugins.cameras.storage" },
          { "zigbee", "db.plugins.zigbee.storage" },
          { "zwave", "db.plugins.zwave.storage" } } );

    return isRenamed;
}

}  // namespace v1_0
}  // namespace MigrationTool
}  // namespace HubPlatform
