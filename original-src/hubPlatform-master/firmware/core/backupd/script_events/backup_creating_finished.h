#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace BackupUpdateManager {

class BackupCreatingFinished : public Services::ScriptEvent {
public:
    explicit BackupCreatingFinished( bool successful );

    Json toJson() const override;

private:
    bool m_successful;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
