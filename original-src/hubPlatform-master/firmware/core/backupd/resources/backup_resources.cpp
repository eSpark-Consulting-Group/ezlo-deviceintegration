#include "backupd/resources/backup_resources.h"

namespace HubPlatform {
namespace BackupUpdateManager {
namespace Resources {

namespace DBNames {
const char* const BackupDbName = HaBackupDbName;
}  // namespace DBNames

const char* const ServiceName = "backupd";
const char* const BackupsDirectoryName = "backups";
const char* const FileNamePrefix = "backup_";
const char* const DownloadedSubdirectory = "downloaded";
const char* const RecoverySubdirectory = "recovery";
const char* const BackupFileExtension = ".json";
const char* const BackupVersion = "0.2";
const char* const BackupTypeNative = "native";
const char* const BackupTypeVera = "vera";

namespace ScriptEvent {
namespace Name {

const char* const BackupCreatingStarted = "backup_creating_started";
const char* const BackupCreatingFinished = "backup_creating_finished";

}  // namespace Name
namespace Fields {

extern const char* const Status;

}  // namespace Fields
}  // namespace ScriptEvent

namespace VeraBackupActions {
const char* const MigrateDevicesAction = "migrate_devices";
const char* const MigrateRoomsAction = "migrate_rooms";
const char* const MigrateScenesAction = "migrate_scenes";
}  // namespace VeraBackupActions

namespace BackupSchedule {
const char* const IdName = "daily_backup";
const char* const DailyBackupRunTime = "daily_backup_runtime";
const char* const PreviousBackupRunTime = "previous_backup_runtime";

}  // namespace BackupSchedule

}  // namespace Resources
}  // namespace BackupUpdateManager
}  // namespace HubPlatform
