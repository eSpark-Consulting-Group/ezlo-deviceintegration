#pragma once

#include "extensions_manager/em_messenger.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class FakeMessenger final : public Messenger {
public:
    void sendProgressStatus( MessageSubClasses processType,
                             std::string status,
                             int percentage,
                             const CommonUtils::PluginSummary& pluginSummary ) override {}
    void sendProgressError( MessageSubClasses processType,
                            Errors::Error error,
                            const CommonUtils::PluginSummary& pluginSummary ) override {}
    void sendExecutePluginScript( const std::string& script,
                                  const Json& scriptParams,
                                  const std::string& operationId = "",
                                  bool isUserRunScript = false,
                                  int timeout = 60 ) override {
        sendExecutePluginScriptSync( script, scriptParams, operationId, isUserRunScript, timeout );
    }

    ScriptResult sendExecutePluginScriptSync( const std::string& script,
                                              const Json& scriptParams,
                                              const std::string& operationId = "",
                                              bool isUserRunScript = false,
                                              int timeout = 60 ) {
        m_scriptsExecutionList.push_back( script );
        return ScriptResult( std::vector< Json >() );
    }

    void sendPingToAddon( const std::string& addonId ) override {}

    bool sendAddLibrary( const std::string& libraryName, const std::string& entryPoint ) override {
        return true;
    }

    bool sendRemoveLibrary( const std::string& libraryName ) override {
        return true;
    }

    void sendPluginRestarted( const std::string& /*pluginId*/ ) override {}
    void sendPluginTeardown( const std::string& /*pluginId*/ ) override {}
    void sendPluginUninstall( const std::string& /*pluginId*/ ) override {}

public:
    std::vector< std::string > m_scriptsExecutionList;
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
