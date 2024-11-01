#pragma once

#include "plugin_utils/plugin_summary.h"
#include "jsonrpc/error_codes.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_wrapper.h"
#include "base_utils/script_result.h"

namespace HubPlatform {
namespace ExtensionsManager {

class Messenger {
public:
    virtual ~Messenger() = default;
    virtual void sendProgressStatus( MessageSubClasses processType,
                                     std::string status,
                                     int percentage,
                                     const CommonUtils::PluginSummary& pluginSummary ) = 0;
    virtual void sendProgressError( MessageSubClasses processType,
                                    Errors::Error error,
                                    const CommonUtils::PluginSummary& pluginSummary ) = 0;
    //  json = {
    //        "some_key": "some_value",
    //        ...
    //    }
    virtual void sendExecutePluginScript( const std::string& script,
                                          const Json& scriptParams,
                                          const std::string& operationId = "",
                                          bool isUserRunScript = false,
                                          int timeout = 60 ) = 0;
    virtual void sendPingToAddon( const std::string& addonId ) = 0;

    virtual ScriptResult sendExecutePluginScriptSync( const std::string& script,
                                                      const Json& scriptParams,
                                                      const std::string& operationId = "",
                                                      bool isUserRunScript = false,
                                                      int timeout = 60 ) = 0;

    virtual bool sendAddLibrary( const std::string& libraryName, const std::string& entryPoint ) = 0;
    virtual bool sendRemoveLibrary( const std::string& libraryName ) = 0;

    virtual void sendPluginRestarted( const std::string& pluginId ) = 0;
    virtual void sendPluginTeardown( const std::string& pluginId ) = 0;
    virtual void sendPluginUninstall( const std::string& pluginId ) = 0;

    void sendExecutePluginScript( const std::string& script ) {
        sendExecutePluginScript( script, JSON_EMPTY_OBJ );
    }

    ScriptResult sendExecutePluginScriptSync( const std::string& script ) {
        return sendExecutePluginScriptSync( script, JSON_EMPTY_OBJ );
    }
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
