#pragma once

#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace BackupUpdateManager {
namespace Resources {

namespace DBNames {
extern const char* const BackupDbName;
}  // namespace DBNames

extern const char* const ServiceName;
extern const char* const BackupsDirectoryName;
extern const char* const FileNamePrefix;
extern const char* const DownloadedSubdirectory;
extern const char* const RecoverySubdirectory;
extern const char* const BackupFileExtension;
extern const char* const BackupVersion;
extern const char* const BackupTypeNative;
extern const char* const BackupTypeVera;

namespace ScriptEvent {
namespace Name {

extern const char* const BackupCreatingStarted;
extern const char* const BackupCreatingFinished;

}  // namespace Name
namespace Fields {

const char* const Status = "status";

}  // namespace Fields
}  // namespace ScriptEvent

namespace VeraBackupActions {
extern const char* const MigrateDevicesAction;
extern const char* const MigrateRoomsAction;
extern const char* const MigrateScenesAction;
}  // namespace VeraBackupActions

namespace BackupSchedule {
extern const char* const IdName;
extern const char* const DailyBackupRunTime;
extern const char* const PreviousBackupRunTime;

}  // namespace BackupSchedule

}  // namespace Resources
}  // namespace BackupUpdateManager
}  // namespace HubPlatform
