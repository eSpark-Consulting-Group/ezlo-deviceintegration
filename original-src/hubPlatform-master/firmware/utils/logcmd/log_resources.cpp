#include "logcmd/log_resources.h"

#include "system_utils/environment.h"

namespace HubPlatform {
namespace LogCmd {
namespace Resources {

const std::string fileLockPath = Environment::getTempDataFolder() + "/logcmd.lock";

}  // namespace Resources
}  // namespace LogCmd
}  // namespace HubPlatform
