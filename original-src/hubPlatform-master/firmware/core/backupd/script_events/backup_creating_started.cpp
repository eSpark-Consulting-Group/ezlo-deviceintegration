#include "backup_creating_started.h"
#include "backupd/resources/backup_resources.h"

namespace HubPlatform {
namespace BackupUpdateManager {

BackupCreatingStarted::BackupCreatingStarted()
: Services::ScriptEvent( Resources::ScriptEvent::Name::BackupCreatingStarted ) {}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
