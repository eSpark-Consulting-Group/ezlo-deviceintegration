#pragma once

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

namespace Status {
extern const char* none;
extern const char* pending;
extern const char* playing;
extern const char* failed;
extern const char* finished;
extern const char* stopped;
}  // namespace Status

namespace Fields {
extern const char* id;
extern const char* pid;
extern const char* path;
extern const char* volume;
extern const char* time;
extern const char* handler;
extern const char* userData;
extern const char* plugin;
extern const char* status;
extern const char* restarted;
extern const char* playbackLeft;
extern const char* singlePlaying;
extern const char* reason;
extern const char* code;
extern const char* text;
}  // namespace Fields

namespace Lua {
namespace Fields {
extern const char* id;
}  // namespace Fields
}  // namespace Lua

namespace Resources {
const int volumeRangeMin = 1;
const int volumeRangeMax = 100;
}  // namespace Resources

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform