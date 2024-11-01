#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace BackupUpdateManager {

class BackupCreatingStarted : public Services::ScriptEvent {
public:
    explicit BackupCreatingStarted();
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
