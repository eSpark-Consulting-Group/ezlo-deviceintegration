#pragma once

#include "service_interfaces/service.h"

#include "extensions_manager/em_messenger.h"
#include "extensions_manager/common/addon_utils.h"
#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/extensions/extensions_manager.h"

#include "cored/remote_interface/cored_interface.h"
#include "update_manager/remote_interface/um_remote_interface.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"

#include "plugin_utils/plugin_utils.h"

namespace HubPlatform {

namespace UT {
class EmServiceTestSuite;
class AnonymousPluginUtilsSuite;
}  // namespace UT

namespace ExtensionsManager {

class Service final : public Services::Service, public Messenger {
    friend class HubPlatform::UT::EmServiceTestSuite;
    friend class HubPlatform::UT::AnonymousPluginUtilsSuite;

public:
    Service();

    int restoreBackup( const std::string& tempBackupFolder ) override;

protected:
    void setUp() override;

    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;

private:
    Service( const std::string& name,
             std::unique_ptr< Services::BusWrapper > impl,
             std::unique_ptr< IDbClient > dbClient );

private:
    // Addons
    void handleAddonEventFromBus( const Services::Message& message );

    void handleAddonStarted( const JsonObject& json );
    void handleAddonStopped( const JsonObject& json );

    // Plugins
    JsonObject handlePluginsAvailable( const JsonObject& json );
    JsonObject handlePluginsUnavailable( const JsonObject& json );
    JsonObject handlePluginsUninstall( const JsonObject& json );

    void handlePluginRun( const JsonObject& json );
    JsonObject handlePluginRunAsync( const JsonObject& json );
    JsonObject handleCustomPluginRegister( const JsonObject& json );
    JsonObject handleCustomPluginUnregister( const JsonObject& json );
    JsonObject handleCustomPluginPause( const JsonObject& json );
    void handleInfoCmdFromBus( const Json& requireRegistered );

    JsonObject handleAnonymousPluginAdd( const JsonObject& json );
    JsonObject handleAnonymousPluginDelete( const JsonObject& json );
    JsonObject handleAnonymousPluginChange( const JsonObject& json );
    JsonObject handleAnonymousPluginRun( const JsonObject& json );
    JsonObject handleAnonymousPluginAcquire( const JsonObject& json );
    JsonObject handleAnonymousPluginRelease( const JsonObject& json );
    JsonObject handleLockRequest( const JsonObject& json );
    JsonObject handleUnlockRequest( const JsonObject& json );
    JsonObject handleBackupCreateRequest( const JsonObject& json );
    JsonObject handlePluginConfigPrioritySet( const JsonObject& json );

private:
    void sendProgressStatus( MessageSubClasses processType,
                             std::string status,
                             int percentage,
                             const CommonUtils::PluginSummary& pluginSummary ) override;
    void sendProgressError( MessageSubClasses processType,
                            Errors::Error error,
                            const CommonUtils::PluginSummary& pluginSummary ) override;
    void sendExecutePluginScript( const std::string& script,
                                  const Json& scriptParams,
                                  const std::string& operationId = "",
                                  bool isUserRunScript = false,
                                  int timeout = 60 ) override;
    using Messenger::sendExecutePluginScript;

    ScriptResult sendExecutePluginScriptSync( const std::string& script,
                                              const Json& scriptParams,
                                              const std::string& operationId = "",
                                              bool isUserRunScript = false,
                                              int timeout = 60 ) override;

    void sendPingToAddon( const std::string& addonId ) override;

    bool sendAddLibrary( const std::string& libraryName, const std::string& entryPoint ) override;
    bool sendRemoveLibrary( const std::string& libraryName ) override;

    void sendPluginRestarted( const std::string& pluginId ) override;
    void sendPluginTeardown( const std::string& pluginId ) override;
    void sendPluginUninstall( const std::string& pluginId ) override;

    void sendProgressStatus( MessageSubClasses processType,
                             const std::string& status,
                             const std::string& operationId,
                             int percentage = 0,
                             const JsonObject& error = JSON_EMPTY_OBJ );
    void sendProgressStatus( MessageSubClasses processType, const JsonObject& data );

private:
    std::unique_ptr< AddonUtils > m_addonUtils;
    std::unique_ptr< PluginUtils > m_pluginUtils;
    std::unique_ptr< IAnonymousPluginUtils > m_anonymousPluginUtils;
    CoreDRemoteInterface::CoreDInterfaceUPtr m_coreDInterface;
    std::unique_ptr< UpdateManager::RemoteInterface > m_updateManager;
    std::unique_ptr< ExtensionsManager > m_pluginsManager;
    std::unique_ptr< ScenesManager::BaseRemoteInterface > m_sceneManagerRemoteInterface;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
