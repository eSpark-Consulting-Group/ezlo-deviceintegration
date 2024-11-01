#include "resources.h"

namespace HubPlatform {
namespace Resources {
namespace DBTableNames {
const char* const CommandRetryStorage = "db.firmware.commands.send.retry.storage";
}

namespace Parameters {
extern const char* const CommandData = "command";
extern const char* const RetriesNumber = "retriesNumber";
extern const char* const Id = "_id";
extern const char* const CommandClassId = "commandClassId";
}  // namespace Parameters
}  // namespace Resources
}  // namespace HubPlatform