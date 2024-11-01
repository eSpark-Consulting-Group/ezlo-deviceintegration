#pragma once

namespace HubPlatform {
namespace Services {
namespace Resources {

constexpr const char* const DefaultModuleName = "default";

namespace HubSettings {
constexpr const char* const LogsLevel = "logs.level";
constexpr const char* const LogsColor = "logs.color";
constexpr const char* const LogsIndent = "logs.indent";
constexpr const char* const LuaDefultTimeout = "lua.default_timeout";
constexpr const char* const LuaMinTimeout = "lua.min_timeout";
}  // namespace HubSettings

}  // namespace Resources
}  // namespace Services
}  // namespace HubPlatform
