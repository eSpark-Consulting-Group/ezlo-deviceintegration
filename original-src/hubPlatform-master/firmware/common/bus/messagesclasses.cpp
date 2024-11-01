#include "messagesclasses.h"
#include "system_utils/logger.h"
#include <iostream>

const EnumToStrMap& getClassToStrMap() {
    static const EnumToStrMap classToStr = { // {ALARM,         "Alarm"},
                                             { LOG_EVENT, "LogEvent" },     { ADAPTER_EVENT, "AdapterEvent" },
                                             { ADAPTER_CMD, "AdapterCmd" }, { CONTROL_EVENT, "ControlEvent" },
                                             { CONTROL_CMD, "ControlCmd" }, { HA_EXCEPTION, "HAException" },
                                             { UI_EVENT, "UIEvent" },       { UI_CMD, "UICmd" },
                                             { EZLO_EVENT, "EZLOEvent" },   { EFFECTS, "Effects" },
                                             { SYSTEM_INFO, "SystemInfo" }, { REQUEST, "Request" },
                                             { RESPONSE, "Response" },      { JSON_RPC, "JSON-RPC" },
                                             { EVENT, "AddonEvent" }
    };
    return classToStr;
}

const EnumToStrMap& getGatewayMethodEventsToStrMap() {
    static const EnumToStrMap gatewayMethodEventsToStr = {
        { METHOD_INVOKED, "invoked" }, { METHOD_STARTED, "started" }, { METHOD_STOPPED, "stopped" },
        { METHOD_TIMEOUT, "timeout" }, { METHOD_FAILED, "failed" },   { METHOD_DONE, "done" },
    };
    return gatewayMethodEventsToStr;
}

const EnumToStrMap& getSubclassToStrMap() {
    static const EnumToStrMap subclassToStr = {
        // events:
        { PLANNING_SOFTWARE_UPDATE, "hub.software.update.actions" },
        { UPDATING_FIRMWARE, "hub.firmware.update.progress" },  // NOTE: legacy, do not use
        { UPDATING_SOFTWARE, "hub.software.update.progress" },
        { RESTORE_PROGRESS, "hub.backup.restore.progress" },
        { BACKUP_PROGRESS, "hub.backup.create.progress" },
        { GLOBAL_LOCK, "hub.global.lock" },
        { GLOBAL_UNLOCK, "hub.global.unlock" },
        { SCENE_CHANGED, "hub.scene.changed" },
        { SCENE_ADDED, "hub.scene.added" },
        { SCENE_DELETED, "hub.scene.deleted" },
        { SCENE_PROGRESS, "hub.scene.run.progress" },
        { GLOBAL_SCENE_CHANGED, "hub.scene.global.changed" },
        { GLOBAL_SCENE_ADDED, "hub.scene.global.added" },
        { GLOBAL_SCENE_DELETED, "hub.scene.global.removed" },
        { GLOBAL_SCENE_PROGRESS, "hub.scene.global.run.progress" },
        { HUB_DEVICE_FIRMWARE_UPDATE_PROGRESS, "hub.device.firmware.update.progress" },
        { HUB_DEVICE_FIRMWARE_DOWNLOAD_PROGRESS, "hub.device.firmware.download.progress" },
        { HUB_DEVICE_CHECKED_PROGRESS, "hub.device.checked" },
        { HA_LOG_MSG, "LogMsg" },
        { HA_LOG_DEBUG, "LogDebug" },
        { HA_LOG_INFO, "LogInfo" },
        { HA_LOG_WARN, "LogWarn" },
        { HA_LOG_ERROR, "LogError" },
        { HA_LOG_FATAL, "LogFatal" },
        { SET_HA_LOG_LEVEL, "SetLogLevel" },
        { GET_HA_LOG_LEVEL, "GetLogLevel" },
        { DEVICE_ADDING_STATE, "DeviceAddingState" },      // TODO remove deprecated
        { DEVICE_ADDING_STARTED, "DeviceAddingStarted" },  // TODO remove deprecated
        { DEVICE_ADDED, "DeviceAdded" },                   // TODO remove deprecated
        { DEVICE_UPDATED, "DeviceUpdated" },               // TODO remove deprecated
        { DEVICE_REMOVED, "DeviceRemoved" },               // TODO remove deprecated
        { DEVICE_FORCE_REMOVED, "DeviceForceRemoved" },    // TODO remove deprecated
        { DEVICE_STATE_CHANGED, "DeviceStateChanged" },    // TODO remove deprecated
        { DEVICE_PENDING_STATE, "DevicePendingState" },
        { DEVICE_PENDING_TIMEOUT, "DevicePendingTimeout" },
        { FO_DEVICE_FORCE_REMOVED, "foDeviceForceRemoved" },
        { FO_DEVICE_REMOVED_WHAT_HAPPENED, "foDeviceRemovedWhatHappened" },
        { FO_DEVICE_ADDING_STATE, "foDeviceAddingState" },
        { FO_GATEWAY_METHOD, "foGatewayMethod" },
        { FAVORITE_ADDED, "FavoriteAdded" },
        { FAVORITE_REMOVED, "FavoriteRemoved" },
        { EZLO_LOCK_STATE, "EzloLockState" },
        { ZWAVE_BUSY, "ZwaveBusy" },  // TODO remove deprecated
        { ZWAVE_NETWORK_DISCOVERED, "ZwaveNetworkDiscovered" },
        { ROUTING_TABLE_DATA, "RoutingTableData" },
        { INCLUDING_ZWAVE_DEVICES_STARTED, "IncludingZwaveDevicesStarted" },  // TODO remove deprecated
        { INCLUDING_ZWAVE_DEVICES_STOPPED, "IncludingZwaveDevicesStopped" },  // TODO remove deprecated
        { EXCLUDING_ZWAVE_DEVICES_STARTED, "ExcludingZwaveDevicesStarted" },  // TODO remove deprecated
        { EXCLUDING_ZWAVE_DEVICES_STOPPED, "ExcludingZwaveDevicesStopped" },  // TODO remove deprecated
        { PREFERENCE_LIST_EVENT, "PreferenceListEvent" },
        { PREFERENCE_CHANGED, "PreferenceChanged" },
        { SYSTEM, "SystemMessage" },
        { RULE_TESTING_STARTED, "RuleTestingStarted" },
        { RULE_TESTING_STOPPED, "RuleTestingStopped" },
        { LOG_FILE_UPLOADED, "LogFileUploaded" },
        { TRACE_UPLOADED, "TraceUploaded" },

        // new events:
        { HUB_GATEWAY_ADDED, "hub.gateway.added" },
        { HUB_GATEWAY_UPDATED, "hub.gateway.updated" },
        { HUB_GATEWAY_REMOVED, "hub.gateway.removed" },
        { HUB_GATEWAY_SETTING_ADDED, "hub.gateway.setting.added" },
        { HUB_GATEWAY_SETTING_UPDATED, "hub.gateway.setting.updated" },
        { HUB_GATEWAY_SETTING_REMOVED, "hub.gateway.setting.removed" },
        { UPDATE_GATEWAY_SETTING_VALUE, "hub.gateway.setting.value.update" },
        { UPDATE_GATEWAY_SETTING_STATUS, "hub.gateway.setting.status.update" },
        { HUB_DEVICE_ADDED, "hub.device.added" },
        { HUB_DEVICE_UPDATED, "hub.device.updated" },
        { HUB_DEVICE_REMOVED, "hub.device.removed" },
        { HUB_ITEM_ADDED, "hub.item.added" },
        { HUB_ITEM_UPDATED, "hub.item.updated" },
        { HUB_ITEM_REMOVED, "hub.item.removed" },
        { HUB_DEVICE_SETTING_ADDED, "hub.device.setting.added" },
        { HUB_DEVICE_SETTING_UPDATED, "hub.device.setting.updated" },
        { HUB_DEVICE_SETTING_REMOVED, "hub.device.setting.removed" },
        { HUB_NETWORK_CONNECTED, "hub.network.connected" },
        { HUB_NETWORK_DISCONNECTED, "hub.network.disconnected" },
        { HUB_SET_COORDINATES, "hub.coordinates.set" },
        { HUB_SET_TIME_ZONE, "hub.timezone.set" },
        { HUB_SYS_TIME_UPDATED, "hub.timezone.updated" },
        { HUB_ROOM_CREATED, "hub.room.created" },
        { HUB_ROOM_EDITED, "hub.room.edited" },
        { HUB_ROOM_DELETED, "hub.room.deleted" },
        { HUB_ROOMS_REORDERED, "hub.room.reordered" },
        { HUB_FAVORITE_ADDED, "hub.favorite.added" },
        { HUB_FAVORITE_REMOVED, "hub.favorite.removed" },
        { HUB_USER_NOTIFICATION, "hub.user.notification" },
        { HUB_ITEM_DICTIONARY_UPDATED, "hub.item.dictionary.updated" },
        { HUB_DEVICE_SETTING_DICTIONARY_UPDATED, "hub.device.setting.dictionary.updated" },
        { HUB_STORAGE_CONTROLLER_TOKEN_UPDATED, "hub.storage_controller_token.updated" },
        { HUB_STORAGE_CONTROLLER_TOKEN_EXPIRED, "hub.storage_controller_token.expired" },
        { HUB_LOGS_ROTATED, "hub.logs.rotated" },

        { EXTENDED_HUB_ITEM_UPDATED, "extended.hub.item.updated" },

        { NETWORK_WIFI_SCAN_PROGRESS, "hub.network.wifi.scan.progress" },
        { HUB_NETWORK_CHANGED, "hub.network.changed" },
        { NETWORK_BROADCAST_IP_ADDRESS_CHANGED, "hub.network.ip.address.changed" },
        { NETWORK_CAMERA_CLOUD_TUNNEL_STATUS_CHANGED, "hub.network.camera.cloud.tunnel.status.changed" },
        { NETWORK_INTERFACE_STATUS_CHANGED, "hub.network.interface.status.changed" },

        // commands:
        { RELOAD_RULES, "ReloadRules" },
        { CALL_METHOD, "CallMethod" },
        { CALL_GATEWAY_METHOD, "CallGatewayMethod" },
        { SET_ITEM_VALUE, "hub.item.value.set" },
        { ADD_ITEM_DICTIONARY_VALUE, "hub.item.dictionary.value.add" },
        { SET_ITEM_DICTIONARY_VALUE, "hub.item.dictionary.value.set" },
        { REMOVE_ITEM_DICTIONARY_VALUE, "hub.item.dictionary.value.remove" },
        { CLEAR_ITEM_DICTIONARY, "hub.item.dictionary.clear" },
        { SET_DEVICE_SETTING_VALUE, "hub.device.setting.value.set" },
        { SET_DEVICE_SETTING_DICTIONARY_VALUE, "hub.device.setting.dictionary.value.set" },
        { RESET_DEVICE_SETTING, "hub.device.setting.reset" },
        { CHECK_DEVICE, "hub.device.check" },

        { START_INCLUDING_ZWAVE_DEVICES, "StartIncludingZwaveDevices" },
        { STOP_INCLUDING_ZWAVE_DEVICES, "StopIncludingZwaveDevices" },
        { START_EXCLUDING_ZWAVE_DEVICES, "StartExcludingZwaveDevices" },
        { STOP_EXCLUDING_ZWAVE_DEVICES, "StopExcludingZwaveDevices" },
        { ZWAVE_CHIP_HARD_RESET, "ZwaveChipHardReset" },
        { ZWAVE_IDENTIFY_EVENT, "hub.addons.zwave.identify" },
        { RULE_EXECUTED, "RuleExecuted" },
        { RULE_EXECUTED_FAILED, "RuleExecutedFailed" },
        { RULE_ADD, "RuleAdd" },
        { RULE_CHANGE, "RuleChange" },

        { RULE_FAVORITE, "FavoriteRuleAdd" },
        { USER_ASSIGN, "UserAssign" },
        { USER_UNASSIGN, "UserUnassign" },
        { SHARED_ROOM_CHANGED, "sharedRoomsChanged" },
        { USER_BIND, "UserBind" },
        { USER_UNBIND, "UserUnbind" },
        { SYSTEM_BROADCAST, "SystemBroadcast" },
        { EZLO_SHARED, "EzloShared" },
        { EZLO_REBOOTED, "EzloRebooted" },
        { HUB_NETWORK_RESET, "hub.network.reset" },
        { HUB_STATE_SET, "hub.state.set" },
        { HUB_PROCESS_RUNNING, "hub.process.running" },
        { HUB_PROCESS_STOPPED, "hub.process.stopped" },
        { HUB_CUSTOM_LED, "hub.custom.led" },
        { HUB_SYSTEM_DATETIME_CHANGED, "hub.system.datetime.changed" },

        // exceptions:
        { ZWAVE_HARD_RESET_FAILED, "ZwaveHardResetFailed" },
        { INCLUDING_ZWAVE_DEVICE_FAILED, "IncludingZwaveDeviceFailed" },
        { EXCLUDING_ZWAVE_DEVICE_FAILED, "ExcludingZwaveDeviceFailed" },
        //
        // media streaming - begin
        { MEDIA_STREAM_BROADCAST_START, "MediaStreamBroadcastStart" },
        { MEDIA_STREAM_BROADCAST_STOP, "MediaStreamBroadcastStop" },
        { MEDIA_STREAM_ADD, "MediaStreamAdd" },
        { MEDIA_STREAM_MODIFY, "MediaStreamModify" },
        { MEDIA_STREAM_REMOVE, "MediaStreamRemove" },
        { MEDIA_STREAM_AVAILABLE, "MediaStreamAvailable" },
        { MEDIA_STREAM_HISTORY_STARTED, "MediaStreamHistoryStarted" },
        { MEDIA_STREAM_HISTORY_STOPPED, "MediaStreamHistoryStopped" },
        { MEDIA_STREAM_HISTORY_FILE, "MediaStreamHistoryFile" },
        { MEDIA_STREAM_STATE, "MediaStreamState" },
        { MEDIA_STREAM_FOUND_ONE_CAMERA, "CamerasFound" },
        //{MEDIA_STREAM_FOUND_ONE_CAMERA_PICTURE, "CameraPictureFound"},
        // media streaming - end
        //
        { UI_PREFERENCE_PROPERTIES_CHANGED, "UIPreferencePropertiesChanged" },
        { NO_SUCH_METHOD, "NoSuchMethod" },

        { CREATE_ROOM, "CreateRoom" },
        { SET_ROOM_NAME, "SetRoomName" },
        { SET_ROOM_ORDER, "SetRoomOrder" },
        { DELETE_ROOM, "DeleteRoom" },
        { DELETE_ROOM_ALL, "DeleteRoomAll" },
        { SET_SERVICE_NOTIFICATIONS, "hub.devices.service.notifications.set" },
        { EXECUTE_SCRIPT, "ExecuteScript" },
        { ADD_PLUGIN_LIBRARY, "AddPluginLibrary" },
        { REMOVE_PLUGIN_LIBRARY, "RemovePluginLibrary" },
        { SET_FEATURE_STATUS, "hub.core.feature.status.set" },
        { SET_HUB_SETTING_VALUE, "hub.core.setting.value.set" },
        { HUB_SETTING_UPDATED, "hub.setting.updated" },
        { RELOAD_GLOBAL_LOG_OPTIONS, "ReloadGlobalLogOptions" },
        { HUB_CLOUD_SUBSCRIPTION_CREATE, "hub.cloud.subscription.create" },
        { HUB_CLOUD_SUBSCRIPTION_DELETE, "hub.cloud.subscription.delete" },
        { HUB_CLOUD_SUBSCRIPTIONS_LIST, "hub.cloud.subscriptions.list" },
        { HUB_CLOUD_SUBSCRIPTION_UPDATE, "hub.cloud.subscription.update" },
        { HUB_CLOUD_SUBSCRIPTION_DISPATCH, "hub.cloud.subscription.dispatch" },
        { HUB_CLOUD_ENVIRONMENT_CHANGED, "hub.cloud.environment.changed" },

        // Walk test
        { MODES_PROTECT_SYSTEM_TEST_START, "hub.modes.protect.system.test.start" },
        { MODES_PROTECT_SYSTEM_TEST_STOP, "hub.modes.protect.system.test.stop" },
        { MODES_PROTECT_SYSTEM_TEST_EXTEND, "hub.modes.protect.system.test.extend" },

        // scenes device group operations
        { DEVICE_GROUP_CREATE, "hub.device.group.create" },
        { DEVICE_GROUP_DELETE, "hub.device.group.delete" },
        { DEVICE_GROUP_UPDATE, "hub.device.group.update" },
        { DEVICE_GROUPS_LIST, "hub.device.groups.list" },
        { DEVICE_GROUP_GET, "hub.device.group.get" },
        { DEVICE_GROUP_FIND, "hub.device.group.find" },
        { DEVICE_GROUPS_DEVICE_ADD, "hub.device.group.device.add" },
        { DEVICE_GROUPS_DEVICE_REMOVE, "hub.device.group.device.remove" },

        // policy operations
        { POLICY_CREATE, "hub.policy.create" },
        { POLICY_DELETE, "hub.policy.delete" },
        { POLICY_UPDATE, "hub.policy.update" },
        { POLICY_GET, "hub.policy.get" },
        { POLICY_LIST, "hub.policies.list" },

        // protect operations
        { PROTECT_SIGNAL_LIST, "hub.protect.signal.list" },
        { PROTECT_SECURITY_CONFIGS_LIST, "hub.protect.security.configs.list" },

        // swinger shutdown operations
        { MODES_SWINGER_SHUTDOWN_LIST, "hub.modes.swinger.shutdown.list" },
        { MODES_SWINGER_SHUTDOWN_RESET, "hub.modes.swinger.shutdown.reset" },
        { MODES_SWINGER_SHUTDOWN_DISABLE_ADD, "hub.modes.swinger.shutdown.disable.add" },
        { MODES_SWINGER_SHUTDOWN_DISABLE_REMOVE, "hub.modes.swinger.shutdown.disable.remove" },
        { MODES_SWINGER_SHUTDOWN_LIMITS_SET, "hub.modes.swinger.shutdown.limits.set" },
        { MODES_SWINGER_SHUTDOWN_LIMITS_RESET, "hub.modes.swinger.shutdown.limits.reset" },

        // scenes item group operations
        { ITEM_GROUP_CREATE, "hub.item.group.create" },
        { ITEM_GROUP_DELETE, "hub.item.group.delete" },
        { ITEM_GROUP_UPDATE, "hub.item.group.update" },
        { ITEM_GROUPS_LIST, "hub.item.groups.list" },
        { ITEM_GROUP_GET, "hub.item.group.get" },
        { DEVICE_ITEM_GROUP_EXPAND, "hub.device.item.group.expand" },

        { SEND_LOG_TO_SERVER, "SendLogToServer" },
        { SEND_LOG_TAIL, "SendLogTail" },
        { SEND_DAILY_LOG, "SendDailyLog" },
        { SEND_TRACE_LOG, "SendTraceLog" },
        { BACKUP_CREATE, "backup.create" },
        { BACKUP_RESTORE, "backup.restore" },
        { BACKUP_LIST, "backup.list" },
        { BACKUP_RESTORE_VERA, "backup.restore.vera" },

        { ENSURE_GATEWAY, "EnsureGateway" },
        { MODIFY_GATEWAY, "ModifyGateway" },
        { SET_GATEWAY_SETTING_VALUE, "hub.gateway.setting.value.set" },
        { ADD_GATEWAY_SETTING, "add.gateway.setting" },
        { REMOVE_GATEWAY_SETTING, "remove.gateway.setting" },
        { START_GATEWAY_PROCESS, "start.gateway.process" },
        { STOP_GATEWAY_PROCESS, "stop.gateway.process" },
        { GATEWAY_PROCESS_STARTED, "gateway.process.started" },
        { GATEWAY_PROCESS_STOPPED, "gateway.process.stopped" },
        { ADD_DEVICE, "AddDevice" },
        { MODIFY_DEVICE, "ModifyDevice" },
        { RENAME_DEVICE, "RenameDevice" },
        { PLACE_DEVICE, "PlaceDevice" },
        { ADD_ITEM, "AddItem" },
        { ADD_DEVICE_SETTING, "AddDeviceSetting" },
        { MODIFY_ITEM, "ModifyItem" },
        { MODIFY_DEVICE_SETTING, "ModifyDeviceSetting" },
        { REMOVE_DEVICE_SYNC, "RemoveDeviceSync" },
        { REMOVE_ITEM, "RemoveItem" },
        { REMOVE_POLICY, "RemovePolicy" },
        { REMOVE_DEVICE_SETTING, "RemoveDeviceSetting" },
        { SET_DEVICE_ARMED, "hub.device.armed.set" },
        { SET_DEVICE_PROTECTCONFIG, "hub.device.protect_config.set" },
        { FORCEREMOVE_DEVICE, "hub.device.force_remove" },
        { UPDATE_DEVICE_FIRMWARE, "hub.device.firmware.update" },

        { UPDATE_DEVICE_STATUS, "hub.device.update_device_status" },

        { ADD_VERA_DEVICE_MAPPING, "AddVeraDeviceMapping" },

        { REQUEST_PROVISION_DATA, "core.request_provision_data" },
        { PERFORM_DB_OPERATION, "core.perform_db_operation" },
        { REQUEST_DB_DATA, "core.request_db_data" },
        { EXECUTE_AT_COMMAND_EMBEDDED, "hub.networkd.execute.at_command.embedded" },

        { NETWORK_COMMUNICATION_TEST, "network.communication.test" },

        { ADDON_STARTED, "hub.extensions.addon.started" },
        { ADDON_STOPPED, "hub.extensions.addon.stopped" },

        { SERVICE_STARTED, "hub.services.service.started" },
        { SERVICE_STOPPED, "hub.services.service.stopped" },

        { SERVICE_INFO_GET, "hub.service.info.get" },

        { SOFTWARE_INFO_REQUEST, "hub.software.info.get" },
        { SOFTWARE_UPDATE_PLAN_REQUEST, "hub.software.update.plan" },
        { SOFTWARE_UPDATE_PLAN_EXECUTE, "hub.software.update.execute" },

        { HUB_RESET_REQUEST, "hub.reset" },

        { SOFTWARE_UPDATE, "hub.software.update" },
        { LUUP_SWITCH, "hub.firmware.luup.switch" },
        { BOOTLOADER_UPDATE, "hub.bootloader.update" },
        { RECOVERY_PARTITION_UPDATE, "hub.recovery_partition.update" },

        { HUB_POWEROFF, "hub.poweroff" },
        { HUB_REBOOT, "hub.reboot" },

        { PLUGINS_AVAILABLE, "hub.extensions.plugins.available" },
        { PLUGINS_UNAVAILABLE, "hub.extensions.plugins.unavailable" },
        { PLUGINS_UNINSTALL, "hub.extensions.plugins.uninstall" },
        { PLUGIN_RUN_REQUEST, "hub.extensions.plugin.run" },
        { PLUGIN_RESTARTED, "hub.extensions.plugin.restarted" },

        { ANONYMOUS_PLUGIN_ADD, "hub.scenes.scripts.add" },
        { ANONYMOUS_PLUGIN_CHANGE, "hub.scenes.scripts.set" },
        { ANONYMOUS_PLUGIN_DELETE, "hub.scenes.scripts.delete" },
        { ANONYMOUS_PLUGIN_RUN, "hub.scenes.scripts.run" },
        { ANONYMOUS_PLUGIN_ACQUIRE, "acquire.anonymous.plugin" },
        { ANONYMOUS_PLUGIN_RELEASE, "release.anonymous.plugin" },

        { CUSTOM_PLUGIN_REGISTER, "custom.plugin.register" },
        { CUSTOM_PLUGIN_UNREGISTER, "custom.plugin.unregister" },
        { CUSTOM_PLUGIN_PAUSE, "custom.plugin.pause" },

        { PLUGIN_CONFIG_PRIORITY_SET, "hub.extensions.config.priority.set" },

        { PLUGIN_RUN_PROGRESS, "hub.extensions.plugin.run.progress" },
        { PLUGIN_UI_BROADCAST, "hub.extensions.plugin.ui_broadcast" },

        { RESPONSE_RESULT, "ResponseResult" },

        { GET_HUB_STATUS, "hub.status.get" },

        { SCENE_CREATE, "scenes.create" },
        { SCENE_EDIT, "scenes.edit" },
        { SCENE_CLONE, "scenes.clone" },
        { SCENE_DELETE, "scenes.delete" },
        { SCENE_DELETE_ALL, "scenes.delete.all" },
        { SCENE_STOP, "scenes.stop" },
        { SCENE_RUN, "scenes.run" },
        { SCENE_ENABLE, "scenes.enabled.set" },
        { SCENE_NOTIFICATION_ADD, "scenes.notification.add" },
        { SCENE_NOTIFICATION_REMOVE, "scenes.notification.remove" },
        { SCENE_ROOM_SET, "scene.room.set" },
        { SCENE_ICON_SET, "scene.icon.set" },
        { SCENE_SET_TEST_CLOCK, "scenes.set.test.clock" },
        { SCENE_TIME_LIST, "scenes.time.list" },
        { SCENE_BLOCKS_LIST, "scenes.blocks.list" },
        { SCENE_BLOCKS_DATA_LIST, "scenes.blocks.data.list" },
        { SCENE_SET_BLOCKS, "scenes.set.blocks" },
        { SCENE_REMOVE_BLOCKS, "scenes.remove.blocks" },
        { SCENE_MODIFY_SCENES, "scenes.modify.scenes" },
        { SCENE_EXPRESSION_VALUE_GET, "scenes.expression.value.get" },
        { SCENE_ACTION_BLOCK_TEST, "scenes.action.block.test" },
        { SCENE_NAME_SET, "scenes.name.set" },
        { SCENE_META_SET, "scenes.meta.set" },
        { SCENE_BLOCK_META_SET, "scenes.block.meta.set" },
        { SCENE_HOUSE_MODES_SET, "scenes.house.modes.set" },
        { SCENE_BLOCK_ENABLED_SET, "hub.scenes.block.enabled.set" },
        { SCENE_BLOCK_STATUS_RESET, "hub.scenes.block.status.reset" },
        { SCENE_STATUS_GET, "hub.scenes.status.get" },
        { SCENE_EXPRESSIONS_SET, "hub.scenes.expressions.set" },
        { SCENE_EXPRESSIONS_DELETE, "hub.scenes.expressions.delete" },
        { SCENE_EXPRESSIONS_LIST, "hub.scenes.expressions.list" },
        { SCENE_EXPRESSION_ADDED, "hub.expression.added" },
        { SCENE_EXPRESSION_CHANGED, "hub.expression.changed" },
        { SCENE_EXPRESSION_DELETED, "hub.expression.deleted" },

        { SET_HOUSE_MODE, "hub.modes.switch" },
        { CANCEL_SWITCHING_HOUSE_MODE, "hub.modes.cancel_switch" },
        { HOUSE_MODE_CANCEL_ENTRY_DELAY, "hub.modes.entry_delay.cancel" },
        { HOUSE_MODE_SKIP_ENTRY_DELAY, "hub.modes.entry_delay.skip" },
        { HOUSE_MODE_SET_ENTRY_DELAY, "hub.modes.entry_delay.set" },
        { HOUSE_MODE_ENTRY_DELAY_RESET, "hub.modes.entry_delay.reset" },
        { HOUSE_MODE_ENTRY_DELAY_CHANGED, "hub.modes.entry_delay.changed" },
        { HOUSE_MODE_ALARMED, "hub.modes.alarmed" },
        { HOUSE_MODE_SWITCHED, "hub.modes.switched" },
        { HOUSE_MODE_PHASE_CHANGED, "hub.mode.phase.changed" },
        { HOUSE_MODE_NOTIFICATIONS_ADDED, "hub.modes.notifications.added" },
        { HOUSE_MODE_NOTIFICATIONS_REMOVED, "hub.modes.notifications.removed" },
        { HOUSE_MODE_NOTIFICATIONS_NOTIFYALL, "hub.modes.notifications.notify_all" },
        { HOUSE_MODE_CHANGED, "hub.modes.changed" },
        { HOUSE_MODE_DISARMED_DEVICES_ADDED, "hub.modes.disarmed_devices.added" },
        { HOUSE_MODE_DISARMED_DEVICES_REMOVED, "hub.modes.disarmed_devices.removed" },
        { HOUSE_MODE_ADD_DISARMED_DEVICE, "hub.modes.disarmed_devices.add" },
        { HOUSE_MODE_REMOVE_DISARMED_DEVICE, "hub.modes.disarmed_devices.remove" },
        { SET_NOTIFICATIONS_HOUSE_MODE, "hub.modes.notifications.set" },
        { HOUSE_MODE_ADD_ALARMS_OFF_DEVICE, "hub.modes.alarms_off.add" },
        { HOUSE_MODE_REMOVE_ALARMS_OFF_DEVICE, "hub.modes.alarms_off.remove" },
        { HOUSE_MODE_ALARMS_OFF_ADDED, "hub.modes.alarms_off.added" },
        { HOUSE_MODE_ALARMS_OFF_REMOVED, "hub.modes.alarms_off.removed" },
        { HOUSE_MODE_ADD_CAMERAS_OFF_DEVICE, "hub.modes.cameras_off.add" },
        { HOUSE_MODE_REMOVE_CAMERAS_OFF_DEVICE, "hub.modes.cameras_off.remove" },
        { HOUSE_MODE_CAMERAS_OFF_ADDED, "hub.modes.cameras_off.added" },
        { HOUSE_MODE_CAMERAS_OFF_REMOVED, "hub.modes.cameras_off.removed" },
        { HOUSE_MODE_ADD_BYPASS_DEVICES, "hub.modes.bypass_devices.add" },
        { HOUSE_MODE_REMOVE_BYPASS_DEVICES, "hub.modes.bypass_devices.remove" },
        { HOUSE_MODE_BYPASS_DEVICES_ADDED, "hub.modes.bypass_devices.added" },
        { HOUSE_MODE_BYPASS_DEVICES_REMOVED, "hub.modes.bypass_devices.removed" },
        { SET_DISARMED_DEFAULT_HOUSE_MODE, "hub.modes.disarmed_default.set" },
        { HOUSE_MODE_PROTECT_SET, "hub.modes.protect.set" },
        { HOUSE_MODE_PROTECT_BUTTONS_SET, "hub.modes.protect.buttons.set" },
        { HOUSE_MODE_PROTECT_BUTTONS_ADDED, "hub.modes.protect.buttons.added" },
        { HOUSE_MODE_PROTECT_BUTTONS_UPDATED, "hub.modes.protect.buttons.updated" },
        { HOUSE_MODE_PROTECT_BUTTONS_REMOVED, "hub.modes.protect.buttons.removed" },

        { HOUSE_MODES_PROTECT_DEVICES_ADD, "hub.modes.protect.devices.add" },
        { HOUSE_MODES_PROTECT_DEVICES_REMOVE, "hub.modes.protect.devices.remove" },
        { HOUSE_MODES_PROTECT_DEVICES_ADDED, "hub.modes.protect.devices.added" },
        { HOUSE_MODES_PROTECT_DEVICES_REMOVED, "hub.modes.protect.devices.removed" },
        { HOUSE_MODES_LOCAL_ALARM_OFF, "hub.modes.local.alarm.off" },
        { HOUSE_MODES_LOCAL_ALARMS_TURNED_OFF, "hub.modes.local.alarmed.turned.off" },
        { HOUSE_MODES_FORCE_DISARM, "hub.modes.force_disarm" },

        { GET_WHAT_HAPPENED, "whathappened" },

        { NETWORK_RESET, "network.reset" },
        { NETWORK_WIFI_SCAN_START, "network.wifi.scan.start" },
        { NETWORK_WIFI_SCAN_STOP, "network.wifi.scan.stop" },
        { NETWORK_WIFI_TRY_CONNECT, "network.wifi.try_connect" },
        { NETWORK_MODEM_TRY_CONNECT, "network.modem.try_connect" },
        { NETWORK_MODEM_PIN_CODE_SET, "network.modem.pin_code.set" },
        { NETWORK_MODEM_PIN_CODE_RESET, "network.modem.pin_code.reset" },
        { NETWORK_MODEM_PIN_CODE_ENTER, "network.modem.pin_code.enter" },
        { NETWORK_MODEM_SIM_LOCK_SET, "network.modem.sim_lock.set" },
        { NETWORK_MODEM_REREGISTER, "network.modem.reregister" },
        { NETWORK_MODEM_CLEAR_FPLMN, "network.modem.clear_fplmn" },
        { NETWORK_ENABLED_SET, "network.enabled.set" },
        { NETWORK_SETTINGS_SET, "network.settings.set" },
        { NETWORK_MODEM_TRY_CONNECT_FAILED, "network.modem.try_connect.failed" },
        { NETWORK_MODEM_PIN_CODE_SET_FAILED, "network.modem.pin_code.set.failed" },
        { NETWORK_MODEM_PIN_CODE_RESET_FAILED, "network.modem.pin_code.reset.failed" },
        { NETWORK_MODEM_PIN_CODE_ENTER_FAILED, "network.modem.pin_code.enter.failed" },
        { NETWORK_MODEM_SIM_LOCK_SET_FAILED, "network.modem.sim_lock.set.failed" },
        { NETWORK_MODEM_REREGISTRATION_FAILED, "network.modem.reregistration.failed" },
        { NETWORK_MODEM_CLEAR_FPLMN_FAILED, "network.modem.clear_fplmn.failed" },
        { NETWORK_STATISTICS_SETTINGS_SET, "network.statistics.settings.set" },
        { NETWORK_STATISTICS_GET, "network.statistics.get" },

        { ZWAVE_SWITCH_MULTILEVEL_LEVEL_CHANGE, "zwave.switch_multilevel_level_change" },
        { ZWAVE_COLOR_SWITCH_LEVEL_CHANGE, "zwave.color_switch_level_change" },
        { ZWAVE_SET_USER_CODE, "zwave.set_user_code" },
        { ZWAVE_SET_MASTER_CODE, "zwave.set_master_code" },
        { ZWAVE_REQUEST_USER_CODE, "zwave.request_user_code" },
        { ZWAVE_REQUEST_USER_CODES, "zwave.request_user_codes" },
        { ZWAVE_REQUEST_USER_CODES_CHECKSUM, "zwave.request_user_codes_checksum" },
        { ZWAVE_REQUEST_MASTER_CODE, "zwave.request_master_code" },
        { ZWAVE_REQUEST_USER_CODES_KEYPAD_MODE, "zwave.request_user_codes_keypad_mode" },
        { ZWAVE_SET_USER_CODES_KEYPAD_MODE, "zwave.set_user_codes_keypad_mode" },
        { ZWAVE_REQUEST_ASSOCIATION_GROUP, "zwave.request_association_group" },
        { ZWAVE_SEND_IDENTIFY, "zwave.send_identify" },
        { ZWAVE_REQUEST_ALL_NODES_VALUES, "zwave.request_all_nodes_values" },
        { ZWAVE_REQUEST_NODE_VALUES, "zwave.request_node_values" },
        { ZWAVE_SET_CONFIGURATION_VALUE, "zwave.set_configuration_value" },
        { ZWAVE_ADD_CONFIGURATION, "zwave.add_configuration" },
        { ZWAVE_REMOVE_CONFIGURATION, "zwave.remove_configuration" },
        { ZWAVE_RESET_CONFIGURATION_VALUE, "zwave.reset_configuration_value" },
        { ZWAVE_RESET_ALL_CONFIGURATION_VALUES, "zwave.reset_all_configuration_values" },
        { ZWAVE_REQUEST_CONFIGURATION_VALUE, "zwave.request_configuration_value" },
        { ZWAVE_SET_TOKEN_VALUE, "zwave.set_token_value" },
        { ZWAVE_SET_TOKEN_VALUE_MULTICAST, "zwave.set_token_value_multicast" },
        { ZWAVE_SET_RGB_VALUE_MULTICAST, "zwave.set_rgb_value_multicast" },
        { ZWAVE_SET_SOUND_SWITCH_CONFIGURATION, "zwave.set_sound_switch_configuration" },
        { ZWAVE_REQUEST_SOUND_SWITCH_CONFIGURATION, "zwave.request_sound_switch_configuration" },
        { ZWAVE_SET_SOUND_SWITCH_TONE_PLAY, "zwave.set_sound_switch_tone_play" },
        { ZWAVE_REQUEST_SOUND_SWITCH_TONE_PLAY, "zwave.request_sound_switch_tone_play" },
        { ZWAVE_SET_VALUE_MULTICAST, "zwave.set_value_multicast" },
        { ZWAVE_SET_SILENT_SENSORS, "zwave.set_silent_sensors" },
        { ZWAVE_ADD_NODE_TO_ASSOCIATION_GROUP, "zwave.add_node_to_association_group" },
        { ZWAVE_REMOVE_NODE_FROM_ASSOCIATION_GROUP, "zwave.remove_node_from_association_group" },
        { ZWAVE_REQUEST_SENSOR_ALARM_VALUE, "zwave.request_sensor_alarm_value" },
        { ZWAVE_STOP_SECURITY_INCLUDE, "zwave.stop_security_include" },
        { ZWAVE_REQUEST_WEEK_DAY_ENTRY_LOCK_SCHEDULE, "zwave.request_week_day_entry_lock_schedule" },
        { ZWAVE_REQUEST_YEAR_DAY_ENTRY_LOCK_SCHEDULE, "zwave.request_year_day_entry_lock_schedule" },
        { ZWAVE_REQUEST_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE, "zwave.request_daily_repeating_entry_lock_schedule" },
        { ZWAVE_SET_WEEK_DAY_ENTRY_LOCK_SCHEDULE, "zwave.set_week_day_entry_lock_schedule" },
        { ZWAVE_SET_YEAR_DAY_ENTRY_LOCK_SCHEDULE, "zwave.set_year_day_entry_lock_schedule" },
        { ZWAVE_SET_DAILY_REPEATING_ENTRY_LOCK_SCHEDULE, "zwave.set_daily_repeating_entry_lock_schedule" },
        { ZWAVE_SET_ENTRY_LOCK_SCHEDULE_ENABLE, "zwave.set_entry_lock_schedule_enable" },
        { ZWAVE_SET_SCHEDULE, "zwave.set_schedule" },
        { ZWAVE_REQUEST_SCHEDULE, "zwave.request_schedule" },
        { ZWAVE_SET_SCHEDULE_STATE, "zwave.set_schedule_state" },
        { ZWAVE_REQUEST_SCHEDULE_STATE, "zwave.request_schedule_state" },
        { ZWAVE_RECONSTRUCT_NETWORK, "zwave.reconstruct_network" },
        { ZWAVE_SET_DOOR_LOCK_MODE, "zwave.set_door_lock_mode" },
        { ZWAVE_REQUEST_DOOR_LOCK_STATE, "zwave.request_door_lock_state" },
        { ZWAVE_SET_DOOR_LOCK_CONFIGURATION, "zwave.set_door_lock_configuration" },
        { ZWAVE_REQUEST_DOOR_LOCK_CONFIGURATION, "zwave.request_door_lock_configuration" },
        { ZWAVE_RESET_METER, "zwave.reset_meter" },
        { ZWAVE_REQUEST_METER, "zwave.request_meter" },
        { ZWAVE_SET_BARRIER_SIGNAL_VALUE, "zwave.set_barrier_signal_value" },
        { ZWAVE_REQUEST_BARRIER_SIGNAL_VALUE, "zwave.request_barrier_signal_value" },
        { ZWAVE_UPDATE_NODES_FIRMWARE, "zwave.update_nodes_firmwares" },
        { ZWAVE_DOWNLOAD_NODE_FIRMWARE, "zwave.download_node_firmware" },
        { ZWAVE_NODES_PROVISIONING_ADDED, "hub.addons.zwave.nodes.provisioning.added" },
        { ZWAVE_NODES_PROVISIONING_UPDATED, "hub.addons.zwave.nodes.provisioning.updated" },
        { ZWAVE_NODES_PROVISIONING_REMOVED, "hub.addons.zwave.nodes.provisioning.removed" },
        { ZWAVE_GET_INFO, "zwave.get_info" },
        { ZWAVE_START_REDISCOVER, "zwave.start_rediscover" },
        { ZWAVE_REQUEST_NOTIFICATION_VALUE, "zwave.request_notification_value" },
        { ZWAVE_REQUEST_ALARM_V1_VALUE, "zwave.request_alarm_v1_value" },
        { ZWAVE_SET_THERMOSTAT_SETBACK_VALUE, "zwave.set_thermostat_setback_value" },
        { ZWAVE_SET_DEVICES_POLL_INTERVALS, "zwave.set_devices_poll_intervals" },
        { ZWAVE_SET_UNRESPONSIVE_DEVICE_IGNORE_INTERVAL, "zwave.set_unresponsive_device_ignore_interval" },
        { ZWAVE_DEFAULT_DEVICE_CONFIGURATIONS_FILE, "zwave.set_default_device_configurations_file" },
        { ZWAVE_CHECK_REACHABLE_STATUS, "zwave.check_reachable_status" },
        { ZWAVE_CHECK_NODE_VALUES, "zwave.check_node_values" },
        { ZWAVE_LOCK_ANTI_THEFT, "zwave.lock_anti_theft" },
        { ZWAVE_LOCK_ANTI_THEFT_DEFAULT, "zwave.lock_anti_theft_default" },
        { ZWAVE_UNLOCK_ANTI_THEFT, "zwave.unlock_anti_theft" },
        { ZWAVE_UNLOCK_ANTI_THEFT_DEFAULT, "zwave.unlock_anti_theft_default" },
        { ZWAVE_ADD_DEVICE_SPECIFIC_CONFIGURATION, "zwave.add_device_specific_configuration" },
        { ZWAVE_SET_THERMOSTAT_MODE, "zwave.set_thermostat_mode" },
        { ZWAVE_ADD_DEVICE_SPECIFIC_ASSOCIATION, "zwave.add_device_specific_association" },
        { ZWAVE_CLEAR_DEVICE_SPECIFIC, "zwave.clear_device_specific" },
        { ZWAVE_SYNCHRONIZE_DEVICE_TIME, "zwave.synchronize_device_time" },
        { ZWAVE_SET_WAKE_UP_INTERVAL, "zwave.set_wake_up_interval" },

        { ZIGBEE_START_INCLUDE, "zigbee.start_include" },
        { ZIGBEE_STOP_INCLUDE, "zigbee.stop_include" },
        { ZIGBEE_REMOVE_NODE, "zigbee.remove_node" },
        { ZIGBEE_SET_ATTRIBUTE_VALUE, "zigbee.set_attribute_value" },
        { ZIGBEE_SEND_COMMAND, "zigbee.send_command" },
        { ZIGBEE_REQUEST_ATTRIBUTE_VALUE, "zigbee.request_attribute_value" },
        { ZIGBEE_REQUEST_USER_CODES, "zigbee.request_user_codes" },
        { ZIGBEE_REQUEST_MFG_SPECIFIC_ATTRIBUTE_VALUE, "zigbee.request_mfg_specific_attribute_value" },
        { ZIGBEE_CONFIGURE_ATTRIBUTE_REPORT, "zigbee.configure_attribute_report" },
        { ZIGBEE_CONFIGURE_MFG_SPECIFIC_ATTRIBUTE_REPORT, "zigbee.configure_mfg_specific_attribute_report" },
        { ZIGBEE_RESET, "zigbee.reset" },
        { ZIGBEE_GET_INFO, "zigbee.get_info" },
        { ZIGBEE_SET_NETWORK_RADIO_CHANNEL, "zigbee.set_network_radio_channel" },
        { ZIGBEE_GET_CHILD_AND_ROUTE_INFO, "zigbee.get_child_and_route_info" },

        { TWOGIG_RESET, "2gig.reset" },
        { TWOGIG_START_DEVICE_DISCOVERING, "2gig.start_device_discovering" },
        { TWOGIG_STOP_DEVICE_DISCOVERING, "2gig.stop_device_discovering" },
        { TWOGIG_ADD_DEVICE, "2gig.add_device" },
        { TWOGIG_REMOVE_DEVICE, "2gig.remove_device" },
        { TWOGIG_CONFIGURE_DEVICE, "2gig.configure_device" },

        { CAMERAS_CREATE_LIVE_RTSP_STREAM, "cameras.create_live_rtsp_stream" },
        { CAMERAS_DELETE_LIVE_RTSP_STREAM, "cameras.delete_live_rtsp_stream" },
        { CAMERAS_CREATE_STREAM_SAVING_TO_CLOUD, "cameras.create_stream_saving_to_cloud" },
        { CAMERAS_DELETE_STREAM_SAVING_TO_CLOUD, "cameras.delete_stream_saving_to_cloud" },
        { CAMERAS_START_STREAM_SAVING_TO_CLOUD, "cameras.start_stream_saving_to_cloud" },
        { CAMERAS_STOP_STREAM_SAVING_TO_CLOUD, "cameras.stop_stream_saving_to_cloud" },
        { CAMERAS_SET_VIDEO_PARTICLE_SETTINGS, "cameras.set_video_particle_settings" },
        { CAMERAS_MAKE_SNAPSHOT_FOR_CLOUD, "cameras.make.snapshot.for.cloud" },
        { CAMERAS_ADD_STREAM, "add.camera.stream" },
        { CAMERAS_REMOVE_STREAM, "remove.camera.stream" },
        { CAMERAS_MAKE_SNAPSHOT_FOR_UI, "get.make.snapshot.for.ui" },
        { CAMERAS_GET_STREAM_INFO, "cameras.get_stream_info" },

        { CAMERAS_BROADCAST_START_RTSP_STREAMING, "cameras.broadcast.start.rtsp.streaming" },
        { CAMERAS_BROADCAST_STOP_RTSP_STREAMING, "cameras.broadcast.stop.rtsp.streaming" },
        { CAMERAS_BROADCAST_REMOVE_STREAM, "cameras.broadcast.remove.stream" },
        { CAMERAS_CREATE_AUDIO_STREAM, "cameras.create_audio_stream" },
        { CAMERAS_DELETE_AUDIO_STREAM, "cameras.delete_audio_stream" },
        { CAMERAS_GET_AUDIO_STREAM_INFO, "cameras.get_audio_stream_info" },

        { HUB_SSH_TUNNEL_SUPPORT_ENABLED_SET, "hub.ssh_tunnel.support.enabled.set" },

        { BUTTON_FIND_HUB, "button.find_hub" },
        { BUTTON_WIFI_RESET, "button.wifi_reset" },
        { BUTTON_POWEROFF, "button.poweroff" },
        { BUTTON_REBOOT, "button.reboot" },
        { BUTTON_SOFT_RESET, "button.soft_reset" },
        { BUTTON_FACTORY_RESET, "button.factory_reset" },

        { HUB_INFO_CHANGED, "hub.info.changed" },
        { LOCAL_SERVER_STARTED, "hub.local_server.started" },
        { HUB_BATTERY_INFO_UPDATED, "hub.battery.info.updated" },
        { HUB_CLOUD_REGISTER_REPEAT_PROGRESS, "hub.cloud.register.repeat.progress" },

        { HUB_OFFLINE_CLOUD_DATA_UPDATE, "hub.offline.cloud.data.update" },
        { HUB_OFFLINE_CLOUD_DATA_UPDATE_PROGRESS, "hub.offline.cloud.data.update.progress" },
        { HUB_OFFLINE_CLOUD_DATA_INFO_GET, "hub.offline.cloud.data.info.get" },

        { CLOUD_MESSAGE, "cloud.message" },
        { CLOUD_REGISTERED, "cloud.registered" },
        { CLOUD_UNREGISTERED, "cloud.unregistered" },
        { CLOUD_DEVICES_SEND_COMMAND, "cloud_devices.send_command" },
        { CLOUD_DEVICES_GATEWAY_COMMAND, "cloud_devices.gateway_command" },

        { ADDON_MESSAGE, "addon.message" },
        { CUSTOM_SERVICE_MESSAGE, "custom.service.message" },

        { ICC_REQUEST, "icc.controller_request_send" },
        { ICC_REGISTER_CONTROLLER_CONNECTION, "icc.controller_connection.register" },
        { ICC_UNREGISTER_CONTROLLER_CONNECTION, "icc.controller_connection.unregister" },

        { EXTERNAL_CONTROLLERS_SUBSCRIBE, "external_controllers.subscribe" },
        { EXTERNAL_CONTROLLERS_UNSUBSCRIBE, "external_controllers.unsubscribe" },
        { EXTERNAL_CONTROLLERS_CONNECTED, "external_controllers.connected" },
        { EXTERNAL_CONTROLLERS_BROADCAST, "external_controllers.broadcast" },

        { BT_START_SERVICE_AP, "bluetooth.start.service.ap" },
        { BT_STOP_SERVICE_AP, "bluetooth.stop.service.ap" },
        { BT_INCOMMING_CONNECTION_ESTABLISHED, "bluetooth.incomming_connection.established" },

        { HOTPLUG_EVENT, "hotplug.event" },
        { SUBSCRIBE_FOR_PLUGIN_EVENTS, "subscribe.for.plugin.events" },
        { CHANGE_SUBSCRIBER_EVENT_FILTERS, "change.subscriber.event.filters" },
        { EXTERNAL_EVENT, "external.event" },

        { SPEAKER_PLAY, "speaker.play" },
        { SPEAKER_STOP, "speaker.stop" },
        { SPEAKER_PLAYING_INFO, "speaker.playing.info" },

        { HTTP_LINKS_CREATE_LINK_URL, "http_links.create_link_url" },
        { HTTP_LINKS_DELETE_LINK_URL, "http_links.delete_link_url" },
        { HTTP_LINKS_GET_LINK_INFO, "http_links.get_link_info" },
        { HTTP_LINKS_CLOSE_CONNECTION, "http_links.close_connection" },
        { HTTP_LINKS_GET_CONNECTION_INFO, "http_links.get_connection_info" },
        { HTTP_LINKS_SEND_DATA, "http_links.send_data" },

        { HUB_VIDEOFORWARD, "hub.videoforward" },

        { HUB_VIDOO_ELEMENTS_EVENT, "hub.vidoo.elements.event" },
        { REQUEST_CLOUD_STORAGE_TOKEN, "RequestCloudStorageToken" },
        { REQUEST_CAMERA_STREAMS, "RequestCameraStreams" },
        { VIDOO_SERVICE_MESSAGE, "VidooServiceMessage" },

        // scenes device group operations broadcasts
        { DEVICE_GROUP_CREATED, "hub.device.group.created" },
        { DEVICE_GROUP_DELETED, "hub.device.group.deleted" },
        { DEVICE_GROUP_UPDATED, "hub.device.group.updated" },

        // scenes item group operations broadcasts
        { ITEM_GROUP_CREATED, "hub.item.group.created" },
        { ITEM_GROUP_DELETED, "hub.item.group.deleted" },
        { ITEM_GROUP_UPDATED, "hub.item.group.updated" },

        // policy operations broadcasts
        { POLICY_CREATED, "hub.policy.created" },
        { POLICY_DELETED, "hub.policy.deleted" },
        { POLICY_UPDATED, "hub.policy.updated" },

        // swinger shutdown operations broadcasts
        { MODES_SWINGER_SHUTDOWN_RESET_DONE, "hub.modes.swinger.shutdown.reset.done" },
        { MODES_SWINGER_SHUTDOWN_DISABLE_ADDED, "hub.modes.swinger.shutdown.disable.added" },
        { MODES_SWINGER_SHUTDOWN_DISABLE_REMOVED, "hub.modes.swinger.shutdown.disable.removed" },
        { MODES_SWINGER_SHUTDOWN_LIMITS_CHANGED, "hub.modes.swinger.shutdown.limits.changed" },

        // hub features broadcasts
        { HUB_FEATURE_ADDED, "hub.feature.added" },
        { HUB_FEATURE_REMOVED, "hub.feature.removed" },
        { HUB_FEATURE_UPDATED, "hub.feature.updated" },

        // NMA Client
        { NMA_CLIENT_CONNECTED, "nma.client.connected.internal" },
        { NMA_CLIENT_DISCONNECTED, "nma.client.disconnected.internal" },
        { NETWORK_COMMUNICATION_TEST_PROGRESS, "hub.network.communication.test.progress" },
        { WALK_TEST_PROGRESS, "hub.modes.system.test.progress" },

        // Addon WebRtc V2 broadcasts
        { ADDON_WEBRTC2_PING_STAT, "hub.webrtc2.ping.stat" },
        { ADDON_WEBRTC2_SERVICE_METRICS, "hub.webrtc2.service.metrics" },

        // Notifications
        { HUB_NOTIFICATIONS_SMTP_SETTINGS_SET, "hub.notifications.smtp.settings.set" },
        { HUB_NOTIFICATIONS_SMTP_SETTINGS_GET, "hub.notifications.smtp.settings.get" },
        { HUB_NOTIFICATIONS_SMTP_SEND, "hub.notifications.smtp.send" },
    };
    return subclassToStr;
}

std::string msgSubclassToStr( int msgClass, int msgSubclass ) {
    std::string result{ "UnknownMsgSubClass" };
    bool ok = false;

    switch( msgClass ) {
    case HA_EXCEPTION:
    case UI_EVENT:
    case ADAPTER_EVENT:
    case CONTROL_EVENT:
    case LOG_EVENT:
    case LOG_CMD:
    case UI_CMD:
    case EZLO_EVENT:
    case CONTROL_CMD:
    case ADAPTER_CMD:
    case JSON_RPC:
    case REQUEST:
    case RESPONSE:
    case EFFECTS:
    case EVENT:
        ok = enumToStr( getSubclassToStrMap(), msgSubclass, result, "UnknownSubClass" );
        if( !ok ) {
            HubPlatform::SystemUtils::logWarn()
                << "msgSubclassToStr, Unknown msgSubclass: " + std::to_string( msgSubclass ) +
                       ", class: " + std::to_string( msgClass )
                << std::endl;
        }
        return result;
    case SYSTEM_INFO:
        return "SystemBroadcast";
    default:
        HubPlatform::SystemUtils::logWarn() << "msgSubclassToStr, Unknown MsgClass: " + std::to_string( msgClass ) +
                                                   ", Subclass: " + std::to_string( msgSubclass )
                                            << std::endl;
        return result;
    }
}

int msgSubclassToKey( const int msgClass, const std::string msgSubclass ) {
    int result = UNKNOWN;
    bool ok = false;
    switch( msgClass ) {
    case HA_EXCEPTION:
    case UI_EVENT:
    case ADAPTER_EVENT:
    case CONTROL_EVENT:
    case EZLO_EVENT:
    case LOG_EVENT:
    case LOG_CMD:
    case UI_CMD:
    case CONTROL_CMD:
    case ADAPTER_CMD:
    case EFFECTS:
    case SYSTEM_INFO:
    case JSON_RPC:
    case REQUEST:
    case RESPONSE:
    case EVENT:
        ok = strToEnum( getSubclassToStrMap(), msgSubclass, result, UNKNOWN );
        break;
    default:
        HubPlatform::SystemUtils::logWarn()
            << "msgSubclassToKey, UnknownMsgClass: " + std::to_string( msgClass ) + ", Subclass: " + msgSubclass
            << std::endl;
        return UNKNOWN;
    }

    if( !ok )
        HubPlatform::SystemUtils::logWarn()
            << "msgSubclassToKey, Unknown msgSubclass: " + msgSubclass + ", class: " + std::to_string( msgClass )
            << std::endl;
    return result;
}

std::string getGatewayMethodEvent( const GatewayMethodEvents gatewayMethodEvent ) {
    auto iter = getGatewayMethodEventsToStrMap().find( gatewayMethodEvent );
    if( iter != getGatewayMethodEventsToStrMap().end() ) {
        return iter->second;
    }

    return "unknown";
}
