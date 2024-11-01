#pragma once

#include <cstddef>

namespace HubPlatform {
namespace ExtensionsManager {
namespace Resources {

namespace DBTableNames {

extern const char* const Plugins;
extern const char* const AnonymousPlugins;
extern const char* const PluginsExtensionsPriorities;
extern const char* const CustomNodes;

}  // namespace DBTableNames

namespace PluginRecord {

extern const char* const Version;
extern const char* const Permissions;
extern const char* const ExecutionPolicy;
extern const char* const Extensions;

namespace ExtensionsRecord {

extern const char* const Plugins;
extern const char* const Id;
extern const char* const Configs;
extern const char* const Name;
extern const char* const Config;
extern const char* const Description;

}  // namespace ExtensionsRecord
}  // namespace PluginRecord

namespace ExecutionPolicyStates {

extern const char* const Unknown;
extern const char* const NewStateEachScriptRun;
extern const char* const RestoreLastScriptState;

}  // namespace ExecutionPolicyStates

extern const char* const PluginScriptsPrefix;

namespace AnonymousPlugin {

extern const char* const AnonymousPluginScriptsPrefix;
extern const std::size_t AnonymousPluginContentMaxSize;
extern const std::size_t AnonymousPluginNameMaxSize;

namespace Fields {

extern const char* const Id;
extern const char* const Name;
extern const char* const Code;
extern const char* const Acquired;
extern const char* const Scenes;
extern const char* const References;

}  // namespace Fields

}  // namespace AnonymousPlugin

}  // namespace Resources
}  // namespace ExtensionsManager
}  // namespace HubPlatform
