#include "jsonrpc/error_codes.h"
#include "base_utils/utils.h"
#include "json_utils/json_utils.h"

#include <map>

namespace {

typedef std::pair< std::string, std::string > DataAndMessage;
typedef std::tuple< Errors::Codes /* code */, std::string /* message */, std::string /* data */ > ErrorData;
typedef std::map< Errors::Codes, std::map< Errors::Error, DataAndMessage > > AllErrors;
typedef std::pair< Errors::Codes, std::string > CodeAndData;
typedef std::map< std::string, CodeAndData > MessageToData;

bool findAtFront( const std::string& str, const std::string& value ) {
    return ( str.find( value ) == 0 );
}

const AllErrors& getAllErrors() {
    static const AllErrors allErrors = {
        { Errors::Codes::APPLICATION_ERROR,
          {
              // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
              // //
              { Errors::USER_DOESNT_HAVE_PERMISSION,
                { "User does not have permission", "user.permission.doesnothave" } },
              { Errors::CLOSED_FOR_ALL, { "Closed for all", "user.permission.doesnothave.all" } },
              { Errors::DEVICES_LOCKED, { "Devices is locked", "user.permission.doesnothave.locked" } },
              { Errors::CANT_EDIT_USER, { "Failed to edit user", "user.permission.doesnothave.edit" } },
              { Errors::CANT_GET_EZLOS_FOR_THIS_USER,
                { "Failed to get ezlos for user", "user.permission.doesnothave.ezlo" } },
              { Errors::CANT_GET_TOKEN_FOR_THIS_USER,
                { "Failed to get token for user", "user.permission.doesnothave.token" } },
              { Errors::UNKNOWN_PERMISSION, { "Unknown permission", "user.permission.unknown" } },
              { Errors::BAD_PASSWORD, { "Bad password", "user.login.badpassword" } },
              { Errors::USER_HAS_LOGGED, { "Already logged in", "user.login.alreadylogged" } },
              { Errors::WRONG_CURRENT_PASSWORD, { "Wrong current password", "user.edit.badpassword" } },
              { Errors::CANT_DELETE_USER, { "Failed to delete user", "user.edit.forbidden.delete" } },
              { Errors::CANT_BLOCK_USER, { "Failed to block user", "user.edit.forbidden.block" } },
              { Errors::CANT_EDIT_EMAIL, { "Failed to edit email", "user.edit.forbidden.email" } },
              { Errors::USER_EXISTS, { "User exists", "user.create.alreadyregistered" } },
              { Errors::OWNER_CANT_BE_UNBOUND, { "Owner Failed to be unbound", "user.ezlo.forbidden.unshare" } },
              { Errors::EMAIL_IS_NOT_CONFIRMED, { "Email is not confirmed", "nma.user.notconfirmed.email" } },
              { Errors::USER_NOT_FOUND, { "User not found", "nma.user.notfound" } },
              { Errors::KEY_NOT_FOUND, { "Key not found", "nma.user.notfound.key" } },
              { Errors::EMAIL_NOT_EXISTS, { "Email not exists", "nma.user.notfound.email" } },
              { Errors::CANT_GET_HASH, { "Failed to get hash", "nma.user.notfound.hash" } },
              { Errors::USER_BLOCKED, { "User blocked", "nma.user.blocked" } },
              { Errors::USER_DELETED, { "User deleted", "nma.user.deleted" } },
              { Errors::USER_DISABLED, { "User disabled", "nma.user.disabled" } },
              { Errors::USER_NOT_LOGGED, { "User not logged", "nma.user.notlogged" } },
              { Errors::EZLO_AUTH_ERROR, { "Bad login or password", "nma.user.badloginorpassword" } },
              { Errors::DB_ERROR, { "DB internal error", "nma.db.error" } },
              { Errors::TOKEN_GENERATE_FAIL, { "Cannot generate token", "nma.token.failed.generate" } },
              { Errors::NO_TOKEN_FOUND, { "No token found", "nma.token.notfound" } },
              { Errors::TOKEN_HAS_NO_USER, { "Token has no user", "nma.token.hasno.user" } },
              { Errors::TOKEN_HAS_NO_SERIAL, { "Token has no serial", "nma.token.hasno.serial" } },
              { Errors::TOKEN_USED, { "Token used", "nma.token.used" } },
              { Errors::TOKEN_EXPIRED, { "Token expired", "nma.token.expired" } },
              { Errors::IMAGE_TOO_LARGE, { "Image too large", "nma.image.toolarge" } },
              { Errors::IMAGE_NOT_FOUND, { "Image not found", "nma.image.notfound" } },
              { Errors::ROOM_NOT_FOUND, { "Room not found", "nma.room.notfound" } },
              { Errors::CANT_PLACE_SUBDEVICE,
                { "Placing subdevice is forbidden", "ezlo.device.forbidden.place.subdevice" } },
              { Errors::EZLO_NOT_REGISTERED, { "Hub not registered", "nma.ezlo.notregistered" } },
              { Errors::EZLO_NOT_FOUND, { "Hub not found", "nma.ezlo.notfound" } },
              { Errors::EZLO_NOT_CONNECTED, { "Hub not connected", "nma.ezlo.notconnected" } },
              { Errors::EZLO_NOT_LINKED, { "Hub not linked", "nma.ezlo.notlinked" } },
              { Errors::CANT_SEND_TO_EZLO, { "Failed to send to Hub", "nma.ezlo.unavailable" } },
              { Errors::EZLO_DELETED, { "Hub deleted", "nma.ezlo.deleted" } },
              { Errors::EZLO_BECAME_ONLINE, { "Hub became online", "nma.ezlo.becameonline" } },
              { Errors::BAD_BACKEND_PASS, { "Bad backend pass", "nma.register.badpassword" } },
              { Errors::EZLO_ALREADY_REGISTERED_ON_NMA,
                { "Hub already registered on NMA", "nma.register.alreadyregistered" } },
              { Errors::NO_BACKUPS, { "Hub Replica is not found", "nma.backup.notfound" } },
              { Errors::ADAPTER_UNSUPPORTED, { "Unsupported adapter type", "nma.adapter.unsupported" } },
              { Errors::ADAPTER_UNAVAILABLE, { "Unavailable adapter", "nma.adapter.unavailable" } },
              { Errors::EZLO_ALREADY_REGISTERED, { "Hub already registered", "ezlo.register.alreadyregistered" } },
              { Errors::CANT_RUN_SCRIPT_AFTER_RESTORE_BACKUP,
                { "Failed to run script after restore backup", "ezlo.restore.internal.startscript" } },
              { Errors::STOP_SCRIPT_RUN_ERROR, { "Stop script run error", "ezlo.restore.internal.stopscript" } },
              { Errors::WRONG_BACKUP_VERSION, { "Wrong backup version", "ezlo.restore.unknown.version" } },
              { Errors::BAD_BACKUP_DATA, { "Bad backup data", "ezlo.restore.bad.data" } },
              { Errors::NO_BACKUP_DATA_PRESENT, { "Backup was never performed", "ezlo.no.backup.data.present" } },
              { Errors::UNCOMPRESS_ERROR, { "Uncompress Error", "ezlo.restore.bad.archive" } },
              { Errors::UNPACKED_SIZE_ERROR, { "Unpacked size Error", "ezlo.restore.bad.size" } },
              { Errors::MD5_ERROR, { "MD5 Error", "ezlo.restore.bad.md5" } },
              { Errors::UNKNOWN_DATA_TYPE, { "Unknown data type", "ezlo.restore.bad.type" } },
              { Errors::NO_ENVM_SECTION_IN_BACKUP, { "No envm section in backup", "ezlo.restore.notfound.envm" } },
              { Errors::NO_ZDDX_SECTION_IN_BACKUP, { "No zddx section in backup", "ezlo.restore.notfound.zddx" } },
              { Errors::NO_ZDDXSIZE_SECTION_IN_BACKUP,
                { "No zddxSize section in backup", "ezlo.restore.notfound.size" } },
              { Errors::NO_HOMEID_SECTION_IN_BACKUP,
                { "No homeId section in backup", "ezlo.restore.notfound.homeId" } },
              { Errors::ENVM_UNCOMPRESS_ERROR, { "ENVM Uncompress Error", "ezlo.restore.bad.envm" } },
              { Errors::ZDDX_UNCOMPRESS_ERROR, { "ZDDX Uncompress Error", "ezlo.restore.bad.zddx" } },
              { Errors::STICK_WRITE_ERROR, { "STICK write Error", "ezlo.restore.unavailable.stick" } },
              { Errors::CANT_DOWNLOAD_FIRMWARE,
                { "Failed to download firmware! Network problems!", "ezlo.update.unavailable.firmware" } },
              { Errors::CANT_UPLOAD_DATA, { "Failed to upload data! Network problems!", "ezlo.upload.failed" } },
              { Errors::CANT_DOWNLOAD_DATA, { "Failed to download data! Network problems!", "ezlo.download.failed" } },
              { Errors::CANT_VERIFIED, { "Failed to verified!", "ezlo.update.bad.firmware" } },
              { Errors::CANT_DECRYPT, { "Failed to decrypt!", "ezlo.update.bad.key" } },
              { Errors::CANT_UNPACK, { "Failed to unpack!", "ezlo.update.bad.archive" } },
              { Errors::UPDATE_ZDDX_ERROR, { "Update ZDDX error", "ezlo.update.internal.zddx" } },
              { Errors::DEPENDENCIES_UPDATE_ERROR,
                { "Dependencies update error", "ezlo.update.internal.dependencies" } },
              { Errors::UPDATE_OPERATIONS_ERROR, { "Update operations error", "ezlo.update.internal.operation" } },
              { Errors::UNEXPECTED_ERROR, { "Unexpected error", "ezlo.update.unexpected" } },
              { Errors::BATTERY_LOW, { "Low battery level", "ezlo.battery.low" } },
              { Errors::SEND_FILE_ERROR, { "Send file Error", "ezlo.logs.internal.sendfile" } },
              { Errors::ERROR_OPENING_FILE, { "Error opening file", "ezlo.logs.notfound.file" } },
              { Errors::SET_LOGLEVEL_ERROR, { "Set logLevel error", "ezlo.logs.internal.setlevel" } },
              { Errors::SEND_LOGS_ERROR, { "Send logs error", "ezlo.log.send" } },
              { Errors::SCRIPT_OPEN_ERROR, { "Script open error", "ezlo.wifi.notfound.script" } },
              { Errors::SCRIPT_ERROR, { "Script error", "ezlo.wifi.internal.script" } },
              { Errors::REMOVE_NET_ERROR, { "Remove net Error!", "ezlo.wifi.internal.remove" } },
              { Errors::ADD_NET_ERROR, { "Add net Error!", "ezlo.wifi.internal.add" } },
              { Errors::CHECK_ERROR, { "Check Error!", "ezlo.wifi.internal.check" } },
              { Errors::RESET_WIFI_ERROR, { "Reset Wi-Fi Error!", "ezlo.wifi.internal.reset" } },
              { Errors::NOT_SENT_TO_GATEWAY, { "Not sent command to gateway", "ezlo.gateway.send.error" } },
              { Errors::GATEWAY_BUSY, { "Gateway busy", "ezlo.adapter.busy.gateway" } },
              { Errors::GATEWAY_COMMAND_IS_EMPTY, { "Gateway command is empty", "ezlo.gateway.command.empty" } },
              { Errors::ZWAVE_BUSY, { "Zwave busy", "ezlo.adapter.busy.zwave" } },
              { Errors::DEVICE_BUSY, { "Device busy", "ezlo.device.busy" } },
              { Errors::DEVICES_NOT_FOUND, { "Devices not found!", "ezlo.devices.notfound" } },
              { Errors::NO_DEVICE_FOUND, { "No device found!", "ezlo.device.notfound" } },
              { Errors::NO_SUBDEVICE_FOUND, { "No subdevice found", "ezlo.device.notfound.subdevice" } },
              { Errors::NO_GATEWAY_FOUND, { "No gateway found!", "ezlo.gateway.notfound" } },
              { Errors::SUCH_NAME_ALREADY_EXISTS, { "Such name already exists!", "ezlo.device.notunique.name" } },
              { Errors::NO_ITEM_FOUND, { "No item found!", "ezlo.item.notfound" } },
              { Errors::DEVICE_CHECK_NOT_SUPPORTED,
                { "The device doesn't support checking values or status.", "hub.device.check.unsupported" } },
              { Errors::BACKUP_CREATE_FAILED, { "Failed to create backup", "hub.backup.create.failed" } },
              { Errors::BACKUP_RESTORE_FAILED, { "Failed to restore backup", "hub.backup.restore.failed" } },
              { Errors::GLOBAL_LOCK_FAILED, { "Global lock failed", "hub.global.lock.failed" } },
              { Errors::SCENE_IS_OVERLAPPING, { "Scene is overlapping with other scene", "ezlo.scenes.overlapping" } },
              { Errors::SCENE_WRONG_WHEN_BLOCK,
                { "Scene is ill-formed. Can't parse when block", "ezlo.scenes.block.when.wrong" } },
              { Errors::SCENE_GLOBAL_RESTRICTION_MULTIPLE_BLOCKS,
                { "Scene is ill-formed. Can't parse global Restriction block",
                  "ezlo.scenes.block.global.restriction.wrong" } },
              { Errors::SCENE_DATE_RANGE_NOT_DEFINED,
                { "Start or End of date range should be defined", "ezlo.scenes.block.when.date.range.not.defined" } },
              { Errors::SCENE_WRONG_TIME_FORMAT, { "Wrong time format", "ezlo.scenes.block.when.time.format" } },
              { Errors::SCENE_WRONG_DATE, { "Wrong date", "ezlo.scenes.block.when.date" } },
              { Errors::SCENE_WRONG_TIME, { "Wrong time", "ezlo.scenes.block.when.time" } },
              { Errors::SCENE_WRONG_THEN_BLOCK,
                { "Scene is ill-formed. Can't parse then block", "ezlo.scenes.block.then.wrong" } },
              { Errors::SCENE_WRONG_ELSE_BLOCK,
                { "Scene is ill-formed. Can't parse else block", "ezlo.scenes.block.else.wrong" } },
              { Errors::SCENE_WRONG_THEN_GROUPS_BLOCK,
                { "Scene is ill-formed. Can't parse thenGroups block", "ezlo.scenes.block.groups.then.wrong" } },
              { Errors::SCENE_WRONG_ELSE_GROUPS_BLOCK,
                { "Scene is ill-formed. Can't parse elseGroups block", "ezlo.scenes.block.groups.else.wrong" } },
              { Errors::SCENE_ACTION_GROUP_NOT_EXIST,
                { "Scene does not contain an action group with such ID", "ezlo.scenes.block.groups.not.exist" } },
              { Errors::SCENE_ACTION_GROUP_EXIST_WRONG_SCENE,
                { "Action group exists but in other scene", "ezlo.scenes.block.groups.exist.wrong.scene" } },
              { Errors::SCENE_ACTION_GROUP_UNUSED,
                { "Scene is ill-formed. An action group is not linked to a when block",
                  "ezlo.scenes.block.groups.unused" } },
              { Errors::SCENE_UNSUPPORTED_ACTION_BLOCK,
                { "Scene is ill-formed. Unsupported action block is found", "ezlo.scenes.block.unsupported" } },
              { Errors::SCENE_IS_USED_NOT_SUPPORTED_METHOD,
                { "Scene is failed. There is no such method", "ezlo.scenes.method.unknown" } },
              { Errors::SCENE_NOT_SUPPORTED_FEATURE_IS_USED,
                { "Can't create scene with unsupported feature.", "ezlo.scenes.feature.not_supported" } },
              { Errors::SCENE_DOES_NOT_EXIST, { "The scene with this id does not exist", "ezlo.scenes.not.exist" } },
              { Errors::SCENE_CANT_BE_CLONED, { "The scene cannot be cloned", "ezlo.scenes.cannot.be.cloned" } },
              { Errors::SCENE_WHEN_SAME_ITEM_IN_AND,
                { "Scene contain conditions for same functionality inside of AND condition",
                  "scenes.when.same_item_in_and" } },
              { Errors::SCENE_WHEN_SAME_BUTTON_IN_AND,
                { "Scene contain conditions for same functionality inside of AND condition",
                  "scenes.when.same_button_in_and" } },
              { Errors::SCENE_WHEN_NOT_INTERSECT_NUMBERS,
                { "Scene contain conditions for not intersect numbers values inside of AND condition",
                  "scenes.when.not_intersect_numbers" } },
              { Errors::SCENE_WHEN_CONTAIN_MORE_THAN_ONE_TIME,
                { "Scene cannot contain more than one \"time\" condition in the same AND operator ",
                  "scenes.when.more_than_one_time" } },
              { Errors::SCENE_WHEN_INTERVAL_WITH_OTHER_TIME_EVENT_IN_AND,
                { "Scene cannot contain \"isInterval\" conditions along with other time based condition in the same "
                  "AND operator",
                  "scenes.when.interval_with_other_time_event_in_and" } },
              { Errors::SCENE_WHEN_TYPE_MISMATCH,
                { "When block has incompatible value types", "scenes.when.type.mismatch" } },
              { Errors::SCENE_WHEN_SOURCE_TYPE_MISMATCH,
                { "When block has mismatched value source types", "scenes.when.source.type.mismatch" } },
              { Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                { "When block cannot have multiple value sources", "scenes.when.multiple.sources.prohibited" } },
              { Errors::SCENE_WHEN_RECURSIVE_EXECUTE,
                { "Scene contain conditions with current scene id", "scenes.when.recursive_execute" } },
              { Errors::SCENE_ACTION_RECURSIVE_EXECUTE,
                { "Scene contain actions with current scene id", "scenes.action.recursive_execute" } },
              { Errors::SCENE_CYCLE_DETECTED,
                { "Requested action will create an infinite loop in scenes execution", "scenes.cycle.detected" } },
              { Errors::SCENE_BLOCK_DOES_NOT_EXIST, { "Scene block does not exist", "scenes.block.not.exist" } },
              { Errors::SCENE_TOGGLE_VALUE_ERROR, { "Scene toggle value error", "scenes.toggle.value.error" } },
              { Errors::SCENE_BLOCK_ID_DUPLICATE, { "Scene has duplicate identifiers", "scenes.block.id.duplicate" } },

              { Errors::REMOTE_PORT_FORWARDING_FAILED,
                { "Remote port forwarding failed: unknown error", "hub.ssh_tunnel.internal_error" } },
              { Errors::STATIC_SERVER_UNAVAILABLE, { "Static server unavailable", "static.token.unavailable" } },
              { Errors::STATIC_SERVER_BAD_JSON, { "Static server bad json", "static.token.bad.json" } },
              { Errors::STATIC_SERVER_BAD_DATA, { "Static server bad data", "static.token.bad.data" } },
              { Errors::NMA_TEMPLATE_EXISTS, { "Template already exists", "nma.template.exists" } },
              { Errors::MEDIA_STREAM_SUBSYSTEM_FAILURE,
                { "Media stream subsystem failed to execute requested operation", "mediastream.failure.error" } },
              { Errors::MEDIA_STREAM_INVALID_PAGE_ID,
                { "Invalid or not existing roomId parameter value specified", "mediastream.invalid.page.id" } },
              { Errors::FFMPEG_FAILURE_ADD,
                { "Failed to add video stream (at least some of streams were not added)", "ffmpeg.failure.add" } },
              { Errors::FFMPEG_FAILURE_REMOVE,
                { "Failed to remove video stream (at least some of streams were not removed)",
                  "ffmpeg.failure.remove" } },
              { Errors::FFMPEG_FAILURE_CFG_LOAD,
                { "Failed to load configuration file for video streaming", "ffmpeg.failure.cfg.load" } },
              { Errors::FFMPEG_FAILURE_CFG_SAVE,
                { "Failed to save configuration file for video streaming", "ffmpeg.failure.cfg.save" } },
              { Errors::FFMPEG_FAILURE_STREAM_REGISTER,
                { "Failed to register video stream (at least some of streams were not registered)",
                  "ffmpeg.failure.stream.register" } },
              { Errors::FFMPEG_FAILURE_STREAM_RUN,
                { "Failed to start video stream process (at least some of streams were not starred)",
                  "ffmpeg.failure.stream.run" } },
              { Errors::FFMPEG_FAILURE_STREAM_STOP,
                { "Failed to stop video stream process (at least some of streams were not stopped)",
                  "ffmpeg.failure.stream.stop" } },
              { Errors::FFMPEG_FAILURE_STREAM_RESTART,
                { "Failed to restart video stream process (at least some of streams were not restarted)",
                  "ffmpeg.failure.stream.restart" } },
              { Errors::FFMPEG_FAILURE_STREAM_STATUS,
                { "At least some of streams do not work as they should", "ffmpeg.failure.stream.status" } },
              { Errors::CAMERA_SUBSYSTEM_INTERNAL_ERROR,
                { "Internal error of camera support subsystem", "camera.system.internal.error" } },
              { Errors::CAMERA_ID_OR_URL_MISSING,
                { "neither cameraId nor cameraUrl specified in call parameters", "camera.id.or.url.missing" } },
              { Errors::CAMERA_BAD_CAMERA_URL, { "Malformed camera URL", "camera.bad.camera.url" } },
              { Errors::CAMERA_BAD_FTP_URL, { "Malformed FTP URL", "camera.bad.ftp.url" } },
              { Errors::CAMERA_BAD_NOT_EXISTING_ID, { "Bad or not existing camera ID", "camera.bad.id" } },
              { Errors::CAMERA_ID_MISSING, { "Missing cameraId parameter", "camera.missing.id" } },
              { Errors::CAMERA_SENDER_MISSING, { "Missing sender parameter", "camera.missing.sender" } },
              { Errors::SHUTDOWN_MODE_ENTERED, { "UId system is entered shutdown mode", "ezlo.uid.shutdown.mode" } },
              { Errors::RPC_CALL_QUEUE_OVERFLOW, { "RPC call queue reached limit", "ezlo.rpc.call.queue.overflow" } },
              { Errors::SPREAD_BUS_IS_NOT_OPERATABLE,
                { "Spread bus was not initialized correctly", "ezlo.spread.bus.was.not.initialized" } },
              { Errors::SPREAD_BUS_ERROR, { "Spread bus internal error", "ezlo.spread.bus.internal.error" } },
              { Errors::OBJECT_NOT_FOUND, { "Object not found", "generic.object.not.found" } },
              { Errors::CONFIRMATION_EMAIL_FAILURE,
                { "Failed to send confirmation email", "nma.confirmation.email.failure" } },
              { Errors::EZLO_HUB_SERIAL_NUMBER_IS_INVALID,
                { "eZLO hub serial number is invalid", "nma.invalid.ezlo.serial" } },
              { Errors::EZLO_REGISTRATION_TIMEOUT, { "Hub registration timeout", "nma.ezlo.registration.timeout" } },
              { Errors::EZLO_MIGRATION_FLOW_ERROR,
                { "Request to cloud coordinator failed", "nma.ezlo.registration.flow.error" } },
              { Errors::EZLO_ASSIGN_ERROR, { "Assigning hub failed", "nma.ezlo.registration.assign.error" } },
              { Errors::PLUGIN_DOWNLOAD_FAILED,
                { "Failed to download plugins!", "extensions.plugin.download.failed" } },
              { Errors::PLUGIN_INSTALL_FAILED, { "Failed to install plugins!", "extensions.plugin.install.failed" } },
              { Errors::PLUGIN_UNINSTALL_FAILED,
                { "Failed to uninstall plugins!", "extensions.plugin.uninstall.failed" } },
              { Errors::PLUGIN_UPGRADE_FAILED, { "Failed to upgrade plugin", "extensions.plugin.upgrade.failed" } },
              { Errors::PLUGIN_RUN_FAILED, { "Failed to run plugin script!", "extensions.plugin.run.failed" } },
              { Errors::ANONYMOUS_PLUGIN_ACQUIRED, { "Anonymous plugin acquired!", "anonymous.plugin.acquired" } },
              { Errors::ANONYMOUS_PLUGIN_DOES_NOT_EXIST,
                { "Anonymous plugin doesn't exist!", "anonymous.plugin.doesnot.exist" } },
              { Errors::ANONYMOUS_PLUGIN_FAILED_TO_ACQUIRE,
                { "Failed to acquire anonymous plugin!", "anonymous.plugin.acquire.failed" } },
              { Errors::CUSTOM_PLUGIN_FAILED_TO_DOWNLOAD,
                { "Failed to download custom plugin!", "custom.plugin.failed.to.download" } },
              { Errors::CUSTOM_PLUGIN_FAILED_TO_DEPLOY,
                { "Failed to deploy custom plugin!", "custom.plugin.failed.to.deploy" } },
              { Errors::CUSTOM_PLUGIN_NO_INSTALLED,
                { "Custom plugin isn't installed!", "custom.plugin.no.installed" } },
              { Errors::CUSTOM_PLUGIN_ALREADY_INSTALLED,
                { "Custom plugin is already installed!", "custom.plugin.already.installed" } },
              { Errors::CUSTOM_PLUGIN_OPERATION_IS_RUNNING,
                { "Custom plugin operation is running!", "custom.plugin.operation.is.running" } },
              { Errors::CUSTOM_PLUGIN_INVALID_METADATA,
                { "Invalid custom plugin metadata!", "custom.plugin.invalid.metadata" } },
              { Errors::CUSTOM_PLUGIN_INVALID_CHECKSUM,
                { "Invalid custom plugin checksum!", "custom.plugin.invalid.checksum" } },
              { Errors::CUSTOM_PLUGIN_DOWNLOADER_BUSY,
                { "Custom plugin downloader is busy!", "custom.plugin.downloader.busy" } },
              { Errors::CUSTOM_PLUGIN_GATEWAY_INVALID,
                { "Custom plugin gateway invalid", "custom.plugin.gateway.invalid" } },
              { Errors::CUSTOM_PLUGIN_ARCHIVE_EMPTY,
                { "Custom plugin archive is empty", "custom.plugin.archive.empty" } },
              { Errors::ENTRY_LIMIT_REACHED, { "Entry limit reached", "entry.limit.reached" } },

              { Errors::NETWORK_INFO_GETTING_ERROR, { "Failed to run command", "network.get.failed" } },
              { Errors::NETWORK_COULD_NOT_CONNECT,
                { "Could not connect to the network", "network.connection.failed" } },
              { Errors::NETWORK_TEST_BAD_REQUEST_ERROR, { "Network test bad request", "network.test.bad.request" } },
              { Errors::NETWORK_TEST_RETURN_STATUS_ERROR,
                { "Network test status error", "network.test.status.error" } },
              { Errors::NETWORK_TEST_CONNECTION_ERROR,
                { "Network test connection error", "network.test.connection.error" } },
              { Errors::NETWORK_TEST_ALREADY_STARTED,
                { "Network test already started", "network.test.already.started" } },
              { Errors::NETWORK_INTERFACE_NOT_UP, { "Interface is not up", "network.interface.not.up" } },
              { Errors::NETWORK_INTERFACE_NOT_CONNECTED,
                { "Network interface not connected", "network.interface.not.connected" } },
              { Errors::NETWORK_HAS_NO_INTERNET,
                { "Network has no internet connection", "network.connection.no_internet" } },
              { Errors::NETWORK_SETTINGS_SET_FAILED,
                { "Failed to set network settings", "network.settings.set.failed" } },
              { Errors::WIFI_SCAN_FAILED, { "Wifi scan failed", "network.wifi.scan.failed" } },
              { Errors::NETWORK_INVALID_INTERFACE, { "Invalid interface id", "network.invalid.interface" } },
              { Errors::MODEM_SIM_NO_PRESENT,
                { "No SIM card present in the specified interface", "network.modem.sim.nopresent" } },
              { Errors::MODEM_INVALID_PIN, { "Invalid pin code", "network.modem.invalid.pin" } },
              { Errors::MODEM_NOALLOWED_OPERATOR, { "Not supported operator", "network.modem.noallowed.operator" } },
              { Errors::MODEM_BAD_AUTH, { "Invalid authentication parameters", " network.modem.bad.auth" } },
              { Errors::MODEM_BAD_INTERNET_PARAMETERS,
                { "Invalid internet connection parameters", "network.modem.bad.internet.parameters" } },
              { Errors::MODEM_SIM_CARD_ERROR, { "Internal SIM card error", "network.modem.sim_card.error" } },
              { Errors::MODEM_SIM_PIN_CODE_IS_REQUIRED,
                { "SIM PIN code is required", "network.modem.sim.pin.required" } },
              { Errors::MODEM_SIM_PUK_CODE_IS_REQUIRED,
                { "SIM PUK code is required", "network.modem.sim.puk.required" } },
              { Errors::MODEM_SIM_PUK_CODE_LAST_ATTEMPT,
                { "Last attempt to enter a PUK code", "network.modem.sim.puk.last.attempt" } },
              { Errors::MODEM_OPERATION_NOALLOWED,
                { "Operation is not allowed", "network.modem.operation.noallowed" } },
              { Errors::MODEM_HUNG, { "Modem doesn't response", "network.modem.hung" } },
              { Errors::MODEM_NO_REGISTRATION,
                { "No registration on a mobile network", "network.modem.no_registration" } },
              { Errors::NETWORK_OPERATION_PROHIBITED, { "Operation is prohibited", "network.operation.prohibited" } },
              { Errors::NETWORK_UNKNOWN_ERROR, { "UNKNOWN reason", "network.unknown" } },

              { Errors::DEVICE_IS_PERSISTENT, { "Device is persistent", "ezlo.device.persistent" } },
              { Errors::FIRMWARE_FEATURE_DISABLED,
                { "Requested operation is rejected due do a disabled feature", "firmware.feature.disabled" } },
              { Errors::SYSTEM_ERROR, { "System error", "system.error" } },
              { Errors::NOT_SUPPORTED, { "Not supported", "not.supported" } },
              { Errors::JSON_PARSE_ERROR, { "Can't parse json", "ezlo.json.parse.error" } },
              { Errors::SCENES_ITEM_DOES_NOT_EXIST, { "Item does not exist", "scenes.item.does.not.exist" } },
              { Errors::SCENES_ITEM_VALUE_TYPE_ERROR, { "Unexpected value type", "scenes.item.value.type.error" } },
              { Errors::SCENES_EXPRESSION_DOES_NOT_EXIST,
                { "Expression doesn't exist", "scenes.expression.does.not.exist" } },
              { Errors::SCENES_EXPRESSION_ERROR, { "Error setting expression", "scenes.expression.error" } },
              { Errors::SCENES_EXPRESSION_TYPE_MISMATCH,
                { "Unexpected value format", "scenes.expression.type.mismatch" } },
              { Errors::SCENES_EXPRESSION_SET_VALUE_ERROR,
                { "Failed to set expression", "scenes.expression.set.value.error" } },
              { Errors::SCENES_EXPRESSION_VALUE_ERROR, { "Expression value error", "scenes.expression.value.error" } },
              { Errors::SCENES_EXPRESSION_PARAMETERS_ERROR,
                { "Expression parameters error", "scenes.expression.parameters.error" } },
              { Errors::SCENES_SET_VARIABLE_ERROR, { "Expression parameters error", "scenes.set.variable.error" } },
              { Errors::SCENE_ACTION_TYPE_MISMATCH, { "Scenes action type mismatch", "scenes.action.type.mismatch" } },

              { Errors::MQTT_CLIENT_NOT_REGISTERED, { "MQTT client not registered", "mqtt.client.not.registered" } },
              { Errors::MQTT_INVALID_CLIENT_NAME, { "Invalid MQTT client name", "mqtt.invalid.client.name" } },
              { Errors::MQTT_MESSAGE_TOO_LONG, { "MQTT message too long", "mqtt.message.too.long" } },
              { Errors::GET_RAM_INFO_ERROR, { "Get RAM info error", "ezlo.ram.info.error" } },
              { Errors::GET_FS_INFO_ERROR, { "Get FS info error", "ezlo.fs.info.error" } },
              { Errors::GET_PROCESSES_INFO_ERROR, { "Get processes info error", "ezlo.processes.info.error" } },

              { Errors::WALK_TEST_ALREADY_STARTED, { "Walk test already started", "walk.test.already.started" } },
              { Errors::WALK_TEST_NOT_STARTED, { "Walk test not started", "walk.test.not.started" } },
              { Errors::WALK_TEST_INCORRECT_OPERATION_ID,
                { "Walk test incorrect operationId", "walk.test.incorrect.id" } },
              { Errors::WALK_TEST_OUTSIDE_EXTEND_PERIOD,
                { "Walk test extend is not permitted outside of the termination period",
                  "walk.test.outside.extend.period" } },
              { Errors::LOCAL_NOTIFICATION_TCDS_DATA_PROCESSING_ERROR,
                { "Local notification tcds data got error during deploying process",
                  "local.notification.tcds.data.processing.error" } },
              { Errors::LOCAL_NOTIFICATION_STORAGE_IS_FULL,
                { "File storage limit has been exceeded", "local.notification.storage.is.full" } },
              { Errors::EZLO_TCDS_UPDATED, { "Tcds data updated", "ezlo.tcds.updated" } },
          } },
        { Errors::Codes::INVALID_REQUEST,
          {
              // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
              { Errors::BAD_REQUEST_HASH_NOT_EXISTS, { "Bad request, hash not exists", "rpc.params.notfound.hash" } },
              { Errors::BAD_PARAMS, { "Bad params", "rpc.params.notfound" } },
              { Errors::UNEXPECTED_REQUEST, { "Unexpected request", "rpc.method.unexpected" } },
          } },
        { Errors::Codes::INTERNAL_ERROR,
          {
              { Errors::BAD_USER_DATA, { "Bad user data", "nma.user.baddata" } },
              { Errors::TOKEN_CREATION_ERROR, { "Token creation error", "nma.token.internal" } },
              { Errors::KEY_GENERATION_ERROR, { "Key generation error", "nma.key.internal" } },
              { Errors::COMPRESS_ERROR, { "Compress error", "ezlo.backup.internal.compress" } },
              { Errors::DATA_SAVE_ERROR, { "Data save error", "ezlo.securechip.internal" } },
              { Errors::MAIL_SENDING_ERROR, { "Mail sending error", "mail.sending.error" } },
              { Errors::INTERNAL_OPERATION_ERROR, { "Internal operation error", "internal.operation.error" } },
              { Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR,
                { "Internal unhandled error", "internal.unhandled.error" } },
              { Errors::LENGTHY_ASYNC_OPERATION_ERROR,
                { "Lengthy async operation error", "lengthy.async.operation.error" } },

              { Errors::SCENE_LOAD_FROM_JSON_IS_FAILED,
                { "Load scene from Json object is failed", "scenes.add.error" } },

              { Errors::UNCANCELABLE_OPERATION_IS_IN_PROGRESS,
                { "Cannot cancel important operation", "cannot.cancel.important.operation" } },
              { Errors::BUS_REQUEST_TIMEOUT, { "Bus request timeout", "bus.request.timeout" } },
              { Errors::BUS_MESSAGE_TOO_LONG, { "Bus message is too long", "bus.message.too.long" } },
              { Errors::BUS_INTERNAL_ERROR, { "Bus internal error", "bus.internal.error" } },
              { Errors::LUA_SCRIPT_ERROR, { "Script error", "ezlo.lua.script.error" } },
              { Errors::LUA_SCRIPT_TIMEOUT, { "Script timeout", "ezlo.lua.script.timeout" } },
              { Errors::LUA_SCRIPT_OPEN_ERROR, { "Script open error", "ezlo.lua.script.open" } },
              { Errors::IMPOSSIBLE_TO_GENERATE_UNIQUE_ID,
                { "The unique id can't be generated", "cannot.generate.unique.id" } },
          } },
        { Errors::Codes::INVALID_PARAMS,
          {
              // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
              // //
              { Errors::SERIAL_TOO_SHORT, { "Serial too short", "rpc.params.tooshort.serial" } },
              { Errors::PASSWORD_IS_TOO_SHORT, { "Password is too short", "rpc.params.tooshort.password" } },
              { Errors::KEY_IS_EMPTY, { "Key is empty", "rpc.params.empty.key" } },
              { Errors::TOKEN_EMPTY, { "Token empty", "rpc.params.empty.token" } },
              { Errors::HASH_IS_EMPTY, { "Hash is empty", "rpc.params.empty.hash" } },
              { Errors::ID_IS_EMPTY, { "id is empty", "rpc.params.empty.id" } },
              { Errors::PARENT_ID_IS_EMPTY, { "parent_id is empty", "rpc.params.empty.parent_id" } },
              { Errors::MESSAGE_HAS_EMPTY_METHOD, { "Message has empty method", "rpc.params.empty.method" } },
              { Errors::WRONG_UNIT_AVAILABLE_UNITS,
                { "Wrong unit, available units: F or C", "rpc.params.unknown.temperatureUnits" } },
              { Errors::UNKNOWN_CONDITION_FILTER,
                { "Unknown condition filter", "rpc.params.unknown.condition_filter" } },
              { Errors::WRONG_PARAMS, { "Wrong params", "rpc.params.invalid" } },
              { Errors::CONSTANT_PARAMS, { "Constant params", "rpc.params.read_only" } },
              { Errors::EMPTY_FIELD, { "Empty field", "rpc.params.empty" } },
              { Errors::WRONG_FIELD, { "Wrong field of object", "rpc.params.wrong_field" } },
              { Errors::HOUSE_MODE_DOES_NOT_EXIST,
                { "The house mode with this id does not exist", "ezlo.house_mode.not.exist" } },
              { Errors::HOUSE_MODE_NAME_DOES_NOT_EXIST,
                { "The house mode with this name does not exist", "ezlo.house_mode.name.not.exist" } },
              { Errors::HOUSE_MODES_AMBIGUOUS_NAME, { "The name is too ambiguous.", "house_mode.ambiguous.name" } },
              { Errors::HOUSE_MODES_INVALID_PHASE,
                { "House Modes aren't in the Entry Delay state", "ezlo.house_mode.alarmed.phase.invalid" } },
              { Errors::HOUSE_MODE_COMMAND_IS_OUTDATED, { "Command is outdated", "ezlo.house_mode.command.outdated" } },
              { Errors::HOUSE_MODE_ID_DOES_NOT_CORRESPOND,
                { "The house mode with this id is not the current house mode", "ezlo.house_mode.id.not.correspond" } },
              { Errors::INVALID_VALUE_SCALE, { "The specified scale cannot be used", "rpc.params.scale.invalid" } },
              { Errors::SCALE_NOT_FOUND, { "The scale not found", "rpc.params.scale.not.found" } },
              { Errors::FAILED_TO_EXECUTE_AT_COMMAND_EMBEDDED,
                { "Failed to execute a custom AT command on an embedded modem",
                  "networkd.failed.execute.at_command.embedded" } },
              { Errors::INVALID_VALUE_RANGE, { "Value is not in valid range", "rpc.params.range.invalid" } },
              { Errors::SEVERITY_DOES_NOT_EXIST, { "Name of severity doesn't exist", "ezlo.severity.not.exist" } },
              { Errors::DELETE_GROUP_IMPOSSIBLE, { "Can't delete persistent group", "ezlo.delete.group.impossible" } },
              { Errors::ALREADY_EXISTS, { "Record with the key already exists", "ezlo.already.exists" } },
              { Errors::DOES_NOT_EXIST, { "Record with the key does not exist", "ezlo.not.exist" } },
              { Errors::TOO_LONG, { "Value is too long", "ezlo.value.too.long" } },
              { Errors::INVALID_SUBSCRIPTION_CAPABILITY_NAME,
                { "Capability value doesn't corresponds to the set standards",
                  "invalid.subscription.capability.name" } },
              { Errors::INVALID_SUBSCRIPTION_VARIABLE_NAME,
                { "Variable value doesn't corresponds to the set standards", "invalid.subscription.variable.name" } },
              { Errors::INVALID_ITEM_NAME,
                { "Item name doesn't corresponds to the set standards", "invalid.item.name" } },
          } },
        { Errors::Codes::METHOD_NOT_FOUND,
          {
              // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
              { Errors::UNKNOWN_METHOD, { "Unknown method", "rpc.method.notfound" } },
          } },
    };  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
    // // // // // // // //
    return allErrors;
}

const ErrorData getUnknown() {
    // Bug here: [1] = data, [2] - message, all other errors: [1] - message, [2] - data
    static const ErrorData unknown( Errors::Codes::APPLICATION_ERROR, "Unknown error", "rpc.unknown.error" );
    return unknown;
}

const ErrorData getErrorData( Errors::Error error ) {
    for( auto& errorGroup : getAllErrors() ) {
        const auto it = errorGroup.second.find( error );
        if( it == errorGroup.second.cend() )
            continue;
        const DataAndMessage& dataAndMessage = ( *it ).second;
        return std::make_tuple( errorGroup.first, dataAndMessage.first, dataAndMessage.second );
    }
    return getUnknown();
}

const MessageToData generateMessageToData() {
    MessageToData messageToData;
    for( auto& code : getAllErrors() )
        for( auto& error : code.second )
            messageToData.insert( { error.second.first, { code.first, error.second.second } } );
    return messageToData;
}

const CodeAndData& getCodeAndData( const std::string& message ) {
    static const MessageToData messageToData = generateMessageToData();
    try {
        return messageToData.at( message );
    }
    catch( ... ) {
        std::cerr << "***** UNKNOWN ERROR MESSAGE: " << message << std::endl;
    }
    static const CodeAndData unknown = { std::get< 0 >( getUnknown() ), std::get< 1 >( getUnknown() ) };
    return unknown;
}

}  // namespace

Errors::Error Errors::Exception::getError() const {
    return m_error;
}

std::string Errors::Exception::getStringRepresentation() const {
    return what();
}

JsonObject Errors::get( Error error ) {
    JsonObject result;
    get( error, result );
    return result;
}

std::string Errors::getStringRepresentation( Error error ) {
    return std::get< 1 >( getErrorData( error ) );
}

std::string Errors::getData( const Error& error ) {
    return std::get< 2 >( getErrorData( error ) );
}

void Errors::makeObject( Codes code, const std::string& data, const std::string& message, JsonObject& result ) {
    json_add_int( result, "code", static_cast< int >( code ) );
    json_add_str( result, "data", data );
    json_add_str( result, "message", message );
}

void Errors::get( Error error, JsonObject& result ) {
    const ErrorData errorData = getErrorData( error );
    const Codes code = std::get< 0 >( errorData );
    const std::string data = std::get< 2 >( errorData );
    const std::string message = std::get< 1 >( errorData );
    makeObject( code, data, message, result );
}

void Errors::getBadRequest( const std::string& fieldName, JsonObject& result ) {
    const std::string data = "rpc.params.notfound." + fieldName;
    const std::string message = "Bad request, " + fieldName + " does not exist";
    makeObject( Codes::INVALID_REQUEST, data, message, result );
}

void Errors::getEmptyString( const std::string& fieldName, JsonObject& result ) {
    const std::string data = "rpc.params.empty." + fieldName;
    const std::string message = "Bad request, " + fieldName + " is empty";
    makeObject( Codes::INVALID_REQUEST, data, message, result );
}

void Errors::getWrongParams( const std::string& fieldName, JsonObject& result ) {
    const std::string data = "rpc.params.invalid." + fieldName;
    const std::string message = "Wrong params, " + fieldName;
    makeObject( Codes::INVALID_PARAMS, data, message, result );
}

void Errors::getInvalidValueRange( const std::string& fieldName, JsonObject& result, const std::string& reason ) {
    const std::string data = "rpc.params.range.invalid." + fieldName;
    const std::string message = "Value is not in valid range";
    makeObject( Codes::INVALID_PARAMS, data, message, result );
    if( !reason.empty() ) {
        result[ "reason" ] = reason;
    }
}

void Errors::getBadObjId( const std::string& id, JsonObject& result ) {
    const std::string data = "nma.db.notvalid.id";
    const std::string message = "Incorrect ObjectID: " + id;
    makeObject( Codes::INTERNAL_ERROR, data, message, result );
}

void Errors::getBadEmail( const std::string& reason, JsonObject& result ) {
    const std::string data = "email.send.internal";
    const std::string message = "Email send: " + reason;
    makeObject( Codes::INTERNAL_ERROR, data, message, result );
}

void Errors::getBadOwner( const std::string& serial, JsonObject& result ) {
    const std::string data = "user.ezlo.hasowner.share";
    const std::string message = "Ezlo serial: " + serial + " has owner already";
    makeObject( Codes::APPLICATION_ERROR, data, message, result );
}

void Errors::getBadStatic( const std::string& reason, JsonObject& result ) {
    const std::string data = "static.token.internal";
    const std::string message = "Static server: " + reason;
    makeObject( Codes::APPLICATION_ERROR, data, message, result );
}

void Errors::getBadEnvm( const std::string& reason, JsonObject& result ) {
    const std::string data = "ezlo.backup.unaviable.envm";
    const std::string message = "Error reading ENVM: " + reason;
    makeObject( Codes::INTERNAL_ERROR, data, message, result );
}

void Errors::update( JsonObject& message ) {
    std::string errorMessage;
    if( !json_get( message, ERROR_FIELD, errorMessage ) )
        return;
    message[ std::string( ERROR_FIELD ) ] = JsonObject();
    JsonObject& error = message[ std::string( ERROR_FIELD ) ];
    json_add_str( error, "message", errorMessage );
    if( findAtFront( errorMessage, "Bad request, " ) ) {
        json_add_int( error, "code", static_cast< int >( Codes::INVALID_REQUEST ) );
        json_add_str( error, "data", "rpc.params.notfound" );
        return;
    }
    if( findAtFront( errorMessage, "Error reading ENVM: " ) ) {
        json_add_int( error, "code", static_cast< int >( Codes::INTERNAL_ERROR ) );
        json_add_str( error, "data", "ezlo.backup.unaviable.envm" );
        return;
    }
    if( findAtFront( errorMessage, "Remote port forwarding failed for listen port " ) ) {
        json_add_int( error, "code", static_cast< int >( Codes::APPLICATION_ERROR ) );
        json_add_str( error, "data", "hub.ssh_tunnel.failed" );
        return;
    }
    if( findAtFront( errorMessage, "Remote port forwarding failed: Wrong password for " ) ) {
        json_add_int( error, "code", static_cast< int >( Codes::APPLICATION_ERROR ) );
        json_add_str( error, "data", "hub.ssh_tunnel.badpassword" );
        return;
    }
    const CodeAndData& codeAndData = getCodeAndData( errorMessage );
    json_add_int( error, "code", static_cast< int >( codeAndData.first ) );
    json_add_str( error, "data", codeAndData.second );
}

void Errors::updateToString( JsonObject& message ) {
    if( !message[ std::string( ERROR_FIELD ) ].is_object() )
        return;
    message[ std::string( ERROR_FIELD ) ] = message[ std::string( ERROR_FIELD ) ][ "message" ];
}
