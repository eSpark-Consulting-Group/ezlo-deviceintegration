
#include "hardware_manager/speaker/model/resources.h"

#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

namespace Status {
const char* none = "none";
const char* pending = "pending";
const char* playing = "playing";
const char* failed = "failed";
const char* finished = "finished";
const char* stopped = "stopped";
}  // namespace Status

namespace Fields {
const char* id = db::ID_FIELD_NAME.c_str();
const char* pid = "pid";
const char* path = "path";
const char* volume = "volume";
const char* time = "time";
const char* handler = "handler";
const char* userData = "user_data";
const char* plugin = "plugin";
const char* status = "status";
const char* restarted = "restarted";
const char* playbackLeft = "playback_left";
const char* singlePlaying = "single";
const char* reason = "reason";
const char* code = "code";
const char* text = "text";
}  // namespace Fields

namespace Lua {
namespace Fields {
const char* id = "id";
}  // namespace Fields
}  // namespace Lua

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform