#include "extensions_manager/resources/em_resources.h"
#include "database_client/db_tables_names.h"
#include "plugin_utils/plugin_utils.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace Resources {

namespace DBTableNames {

const char* const Plugins = "temp.db.firmware.extensions_manager.plugins";
const char* const AnonymousPlugins = "db.firmware.extensions_manager.anonymous_plugins";
const char* const PluginsExtensionsPriorities = "db.firmware.extensions_manager.plugins_extensions_priorities";
const char* const CustomNodes = HaExtensionsManagerCustomNodesDBName;

}  // namespace DBTableNames

namespace PluginRecord {

const char* const Version = "version";
const char* const Permissions = "permissions";
const char* const ExecutionPolicy = "executionPolicy";
const char* const Extensions = "extensions";

namespace ExtensionsRecord {

const char* const Plugins = "plugins";
const char* const Id = "id";
const char* const Configs = "configs";
const char* const Name = "name";
const char* const Config = "config";
const char* const Description = "description";

}  // namespace ExtensionsRecord
}  // namespace PluginRecord

namespace ExecutionPolicyStates {

const char* const Unknown = "unknown";
const char* const NewStateEachScriptRun = "newStateEachScriptRun";
const char* const RestoreLastScriptState = "restoreLastScriptState";

}  // namespace ExecutionPolicyStates

const char* const PluginScriptsPrefix = PluginUtils::m_pluginScriptsPrefix;

namespace AnonymousPlugin {

const char* const AnonymousPluginScriptsPrefix = "anonymous:";
const std::size_t AnonymousPluginContentMaxSize = 5120;  // 5 kb
const std::size_t AnonymousPluginNameMaxSize = 30;

namespace Fields {

const char* const Id = "pluginId";
const char* const Name = "name";
const char* const Code = "code";
const char* const Acquired = "acquired";
const char* const Scenes = "scenes";
const char* const References = "references";

}  // namespace Fields

}  // namespace AnonymousPlugin

}  // namespace Resources
}  // namespace ExtensionsManager
}  // namespace HubPlatform
