#include "backup_creating_finished.h"
#include "backupd/resources/backup_resources.h"

namespace HubPlatform {
namespace BackupUpdateManager {

BackupCreatingFinished::BackupCreatingFinished( bool successful )
: Services::ScriptEvent( Resources::ScriptEvent::Name::BackupCreatingFinished )
, m_successful( successful ) {}

Json BackupCreatingFinished::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Fields::Status ] = m_successful ? "success" : "failed";
    return result;
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
