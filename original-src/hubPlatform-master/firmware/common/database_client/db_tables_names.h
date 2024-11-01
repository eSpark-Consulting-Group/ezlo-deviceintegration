#pragma once

constexpr char StorageFileName[] = "storage.bin";
constexpr char TemporaryStorageFileName[] = "temp.storage.bin";

constexpr char TemporaryTableNamePreffix[] = "temp.";

constexpr char AclPermissionDBName[] = "db.firmware.common.aclPermission";
constexpr char DbVersionsDBName[] = "db.firmware.common.dataVersions";
constexpr char EzloPropertyDBName[] = "db.firmware.uid.firmwareProperty";
constexpr char HaCacheDBName[] = "db.firmware.uid.cache";
constexpr char HaTempCacheDBName[] = "temp.db.firmware.uid.cache";
constexpr char HaDataItemsDBName[] = "db.firmware.uid.dataitems";
constexpr char HaDevicesDBName[] = "db.firmware.cored.devices";
constexpr char HaTempDevicesDBName[] = "temp.db.firmware.cored.devices";
constexpr char HaFavoriteDBName[] = "db.firmware.uid.favorites";
constexpr char HaFavoriteItemsDBName[] = "db.firmware.uid.subDevicesFavorites";
constexpr char HaGatewaysDBName[] = "db.firmware.cored.gateways";
constexpr char HaItemsDBName[] = "db.firmware.uid.items";
constexpr char HaTempItemsDBName[] = "temp.db.firmware.uid.items";
constexpr char HaDeviceSettingsDBName[] = "db.firmware.cored.device_settings";
constexpr char HaGatewaySettingsDBName[] = "db.firmware.cored.gateway_settings";
constexpr char HaHouseModesDBName[] = "db.firmware.cored.houseModes";
constexpr char HaTempHouseModesAlarmDevicesDBName[] = "temp.db.firmware.cored.houseModes.alarmedDevices";
constexpr char HaTempHouseModesAlarmPhaseDBName[] = "temp.db.firmware.cored.houseModes.alarm.phase";
constexpr char HaTempHouseModesSwitchToPhaseDBName[] = "temp.db.firmware.cored.houseModes.switchTo.phase";
constexpr char HaHouseModesCommandTimestampsDBName[] = "db.firmware.cored.houseModes.command.timestamps";
constexpr char HaTempHouseModesCommandTimestampsDBName[] = "temp.db.firmware.cored.houseModes.command.timestamps";
constexpr char HaTempHouseModesDBName[] = "temp.db.firmware.cored.houseModes";
constexpr char HaHouseModesSecurityThreatsDBName[] = "db.firmware.cored.houseModes.securityThreats";
constexpr char HaHouseModesProtectButtonsDBName[] = "db.firmware.cored.houseModes.protectButtons";
constexpr char HaHouseModesCamerasDBName[] = "db.firmware.cored.houseModes.cameras";
constexpr char HaHouseModesProtectDevicesDBName[] = "db.firmware.cored.houseModes.protectDevices";
constexpr char HaUpdateManagerPendingUpdate[] = "db.firmware.update.manager.pendingUpdate";
constexpr char HaUpdateManagerUpdateLog[] = "db.firmware.update.manager.update.log";
constexpr char HaScenesDBName[] = "db.firmware.scenes_manager.rules";
constexpr char HaScenesBlocksDBName[] = "db.firmware.scenes_manager.blocks";
constexpr char HaScenesControllerSubscriptionDBName[] = "db.firmware.scenes_manager.controller.subscription";
constexpr char HaTempScenesItemsDBName[] = "temp.db.firmware.scenes_manager.items";
constexpr char HaTempScenesOptionItemsDBName[] = "temp.db.firmware.scenes_manager.option_items";
constexpr char HaScenesExpressionsDBName[] = "db.firmware.scenes_manager.expressions";
constexpr char HaTempScenesExpressionValuesDBName[] = "temp.db.firmware.scenes_manager.expression.values";
constexpr char HaScenesItemCacheDBName[] = "db.firmware.scenes_manager.item.cache";
constexpr char HaTempSceneBlockStatesDBName[] = "temp.db.firmware.scenes_manager.block_states";
constexpr char HaTempSceneStatesDBName[] = "temp.db.firmware.scenes_manager.scene_states";
constexpr char HaSceneBlockStatesDBName[] = "db.firmware.scenes_manager.block_states";
constexpr char HaSceneStatesDBName[] = "db.firmware.scenes_manager.scene_states";
constexpr char HaTcdsSystemMeshbotsDBName[] = "db.firmware.scenes_manager.tcds.system_meshbots";
constexpr char HaTempDBSceneExecutionContextDBName[] = "temp.db.firmware.scenes_manage.execution_context";
constexpr char HaRoomsOrderDBName[] = "db.firmware.cored.roomsOrder";
constexpr char HaUiPreferencesDBName[] = "db.firmware.uid.preferences";
constexpr char HaUiRuleOrderDBName[] = "db.firmware.uid.ruleOrder";
constexpr char SysFlagsDBName[] = "db.firmware.common.flags";
constexpr char SysOwnerDBName[] = "db.firmware.uid.owner";
constexpr char CredentialsDBName[] = "db.firmware.uid.credentials";
constexpr char ControllersInfoDBName[] = "db.firmware.uid.controllers_info";
constexpr char ControllersMdnsInfoDBName[] = "db.firmware.uid.controllers_mdns_info";
constexpr char RegisteredControllersDBName[] = "db.firmware.uid.registered_controllers";
constexpr char HUBSettings[] = "db.firmware.cored.settings";
constexpr char HUBTempSettings[] = "temp.db.firmware.cored.settings";
constexpr char HaRoomsDBName[] = "db.firmware.cored.rooms";
constexpr char CameraStreamsDBName[] = "db.addons.cameras.streams";
constexpr char LogWhatHappened[] = "db.firmware.cored.devicelog";
constexpr char CoreFeaturesDBName[] = "db.firmware.cored.features";
constexpr char CoreSettingsDBName[] = "db.firmware.cored.internal_settings";
constexpr char HaCloudSubscriptionsDBName[] = "db.firmware.cloud.subscriptions";
constexpr char TempCoreGatewayProcessesDBName[] = "temp.db.firmware.cored.gateway_processes";
constexpr char WifiSettingsDBName[] = "db.firmware.networkd.wifiSettings";
constexpr char InterfaceSettingsDBName[] = "db.firmware.networkd.interfaceSettings";
constexpr char GSMManagerInterfacesDBName[] = "db.firmware.networkd.gsmManager.interfaces";
constexpr char SIMSettingsDBName[] = "db.firmware.networkd.gsmManager.simSettings";
constexpr char NetworkGSMModemActiveSimCardsDBName[] = "db.firmware.networkd.gsmModemActiveSimCards";
constexpr char NetworkStaticInfoDBName[] = "db.firmware.networkd.staticInfo";
constexpr char TempNetworkCloudSshTunnelsDBName[] = "temp.db.firmware.networkd.cloud.ssh.tunnels";
constexpr char NetworkCloudSshTunnelsSettingsDBName[] = "db.firmware.networkd.cloud_ssh_tunnels_settings";
constexpr char TempNetworkDynamicInfoDBName[] = "temp.db.firmware.networkd.dynamicInfo";
constexpr char TempNetworkInterfaces[] = "temp.db.firmware.networkd.interfaces";
constexpr char TempNetworkInfoDBName[] = "temp.db.firmware.networkd.info";
constexpr char NetworkStatisticSettingsDBName[] = "db.firmware.networkd.statistics.settings";
constexpr char NetworkStatisticDBName[] = "db.firmware.networkd.statistics";
constexpr char HaExtensionsManagerCustomNodesDBName[] = "db.firmware.extensions_manager.custom.nodes";
constexpr char HaDeviceGroupsDBName[] = "db.firmware.cored.device_groups";
constexpr char HaItemGroupsDBName[] = "db.firmware.cored.item_groups";
constexpr char HouseModesSwingerShutdown[] = "db.firmware.cored.houseModes.swingerShutdown";
constexpr char TempNetworkManualCommunicationTestStatus[] = "temp.db.firmware.network.manual.communication.test.status";
constexpr char HubErrorsDBName[] = "db.firmware.common.hub_errors";

constexpr char TempVeraDeviceMappingsDBName[] = "temp.db.firmware.cored.veraDeviceMappings";

constexpr char TempPostponedMessagesDBName[] = "temp.db.firmware.uid.postponedMessages";
constexpr char TempCloudStorageDBName[] = "temp.db.firmware.uid.cloud.storage";
constexpr char TempSupportTunnelProcessIdsDBName[] = "temp.db.firmware.support.cloud.ssh.tunnels.processIds";
constexpr char TempTunnelProcessIdsDBName[] = "temp.db.firmware.cloud.ssh.tunnels.processIds";

constexpr char BatteryDefaultDBName[] = "db.firmware.hardware_manager.battery.default";
constexpr char BatteryLearnedDBName[] = "db.firmware.hardware_manager.battery.learned";
constexpr char BatteryInfoDBName[] = "db.firmware.hardware_manager.battery.info";
constexpr char TempBatteryInfoDBName[] = "temp.db.firmware.hardware_manager.battery.info";
constexpr char NetlinkUSBDevicesDBName[] = "db.firmware.hardware_manager.netlink_manager.usb_devices";
constexpr char TempPlayingInfoDBName[] = "temp.db.firmware.hardware_manager.sounds";
constexpr char TempPlayingInfoStartedDBName[] = "temp.db.firmware.hardware_manager.sounds.started";
constexpr char TempHubStateDBName[] = "temp.db.firmware.hardware_manager.hub_state";
constexpr char TempFirmwareCoredSwingerShutdownDevices[] = "temp.db.firmware.cored.swinger.shutdown.devices";
constexpr char FirmwareCoredSwingerShutdownDevices[] = "db.firmware.cored.swinger.shutdown.devices";
constexpr char FirmwareCoredSwingerShutdownTimeSeries[] = "db.firmware.cored.swinger.shutdown.time.series";
constexpr char TempFirmwareCoredSwingerShutdownTimeSeries[] = "temp.db.firmware.cored.swinger.shutdown.time.series";

constexpr char ZigbeeFlasherDBName[] = "db.firmware.utils.zigbee_flasher.storage";

constexpr const char UiExtensionsHttpLinksDBName[] = "db.firmware.uid.ext.httpLinks";
constexpr const char TempUiExtensionsHttpLinksConnectionsDBName[] = "temp.db.firmware.uid.ext.httpLinks.connections";
constexpr const char UiExtensionsCamerasDBName[] = "db.firmware.uid.ext.cameras";
constexpr const char TempCameraSnapshotCacheDBName[] = "temp.db.firmware.camera.snapshot.cache";
constexpr const char TempCameraSnapshotRequestsQueueDBName[] = "temp.db.firmware.camera.snapshot.requests.queue";

constexpr char TempSystemWalkTestProgress[] = "temp.db.firmware.system.walk.test.progress";

constexpr char FirmwareConnectionReporter[] = "temp.db.firmware.connection.reporter";

constexpr char HaBackupDbName[] = "db.firmware.backupd.schedule";

constexpr char HaContactIdsDbName[] = "db.firmware.cored.contact.ids";
constexpr char HaSecurityConfigsDbName[] = "db.firmware.cored.security.configs";

constexpr char HaPoliciesDbName[] = "db.firmware.cored.policies";

constexpr char HaLocalNotificationsDataDbName[] = "db.firmware.local.notifications.data";
constexpr char HaNotificationsSettingsDbName[] = "db.firmware.notifications.settings";
constexpr char HaMeshbotsTemplatesDbName[] = "db.firmware.meshbots.templates";

constexpr auto TableNamesSize = 500U;  // should equal or more than database tables quantity
