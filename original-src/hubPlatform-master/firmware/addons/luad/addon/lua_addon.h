#pragma once

#include "luad/addon/lua_status_data.h"
#include "luad/addon/lua_event_emitter.h"
#include "luad/addon/metrics_event_emitter.h"
#include "luad/addon/lua_metrics.h"
#include "luad/interpreter/module_loader/lua_module_loader.h"

#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "config_reader/config_reader.h"
#include "addon_interfaces/addon.h"
#include "base_utils/script_result.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include "binding_interfaces/common/plugin_status_events_subscription.h"

#include <string>

namespace HubPlatform {

class LuaInterpreter;

namespace Luad {
class TimerManager;
namespace UT {
class LuaTestSuite;
}
}  // namespace Luad
namespace LuaAddon {

class LuaAddon final : public Addons::Addon,
                       public LuaPluginResourcesProvider,
                       public EventEmitter,
                       public Luad::IPluginStatusEventsSubscription,
                       MetricsEventEmitter {
    friend class Luad::UT::LuaTestSuite;

    using LuaInterpreterPtr = std::shared_ptr< LuaInterpreter >;
    struct PluginExecutionState {
        ExtensionsManager::ExecutionPolicy policy = ExtensionsManager::ExecutionPolicy::NewStateEachScriptRun;
        LuaInterpreterPtr interpreter;
    };

    using PluginsLuaInterpretersCollection = std::map< std::string, PluginExecutionState >;

public:
    static const char* const Name;
    static const char* const ExecuteScriptMethod;
    static const char* const ScriptFieldName;
    static const char* const ScriptParamsFieldName;
    static const char* const LibraryNameFieldName;
    static const char* const LibraryEntryPointFieldName;
    static const char* const PluginId;
    static constexpr int DefaultScriptTimeout = 60;
    static constexpr int MinScriptTimeout = 30;

    using PluginStatusCallbacksCollection = std::vector< PluginStatusChangedCallback >;

public:
    LuaAddon();
    LuaAddon( std::unique_ptr< Services::BusWrapper > busWrapper, std::unique_ptr< IDbClient > dbClient );

    ~LuaAddon() override;

    CommonUtils::AddonVersion getVersion() const override;

    void setUp() override;

    void tearDown() override;

    Luad::TimerManager* getTimerManager();

    /* @brief localEmit can be used by lua modules to emit events locally
     *
     * @params[in] eventHandler - name of a script-handler
     * @params[in] eventType - type of an event
     * @params[in] eventData - data specific to the event
     *
     * @note we still have `this->emit()` to notify OTHER addons about our (luaD) events */
    void localEmit( const std::string& eventHandler, const std::string& eventType, const Json& eventData ) override;
    void localEmit( const std::string& eventHandler, const Json& eventData ) override;

    void localEmit( const std::string& eventHandler,
                    const std::string& eventType,
                    LuaScriptParamsHolderPtr scriptParams ) override;
    void localEmit( const std::string& eventHandler, LuaScriptParamsHolderPtr scriptParams ) override;

    void executeScript( const Json& params, LuaScriptParamsHolderPtr scriptParams );
    ScriptResult executeScriptSync( const Json& params, LuaScriptParamsHolderPtr scriptParams );

    bool isPluginScript( const std::string& scriptId ) const override;
    bool isAnonymousPluginScript( const std::string& scriptId ) const override;
    std::unique_ptr< std::istream > getScript( const std::string& scriptId ) override;
    std::string getScriptFilePath( const std::string& scriptId ) const override;
    std::string getDataFilePath( const std::string& scriptId ) const override;
    std::string getPluginIdByScriptId( const std::string& scriptId ) const override;
    bool checkPluginPermission( const std::string& pluginId, const std::string& permission ) const override;

    void sendProgressStatus( const StatusData& statusData, const std::string& errorMessage = "" );

    void subscribeOnPluginRestarted( PluginStatusChangedCallback callback ) override;
    void subscribeOnPluginTeardown( PluginStatusChangedCallback callback ) override;
    void subscribeOnPluginUninstall( PluginStatusChangedCallback callback ) override;

    JsonObject getServiceInfo() override;

public:
    /** Metrics events to count events */
    void emitInterpreterCreated( std::string pluginId ) override;
    void emitScriptStarted( std::string plugin, std::string script, std::string operationId ) override;
    void emitScriptFinished( std::string plugin,
                             std::string script,
                             std::string operationId,
                             Duration duration ) override;
    void emitScriptFailed( std::string plugin,
                           std::string script,
                           std::string operationId,
                           Duration duration ) override;
    void emitScriptAborted( std::string plugin, std::string script, std::string operationId ) override;
    void emitScriptTimeout( std::string plugin,
                            std::string script,
                            std::string operationId,
                            Duration duration ) override;
    void emitPluginResourceUsage( std::string pluginId, std::string policy, int kiloBytes ) override;

private:
    LuaInterpreterPtr createInterpreterForPlugin( const std::string& pluginId );
    LuaInterpreterPtr findInterpreterForPlugin( const std::string& pluginId );
    ExtensionsManager::ExecutionPolicy fetchPluginPolicy( const std::string& pluginId );

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;

    void handleAddLibrary( const JsonObject& params );
    void handleRemoveLibrary( const JsonObject& params );
    void handleSaveSetting( const JsonObject& params );

    void handleEventFromBus( const Services::Message& message );
    void handlePluginRestarted( const JsonObject& params );
    void handlePluginTeardown( const JsonObject& params );
    void handlePluginUninstall( const JsonObject& params );

    void processAllCallbacks( threads::UniqueSafeWrapper< PluginStatusCallbacksCollection >& safeCallbacks,
                              const std::string& pluginId );

    void cleanPluginInfo( const std::string& pluginId );

    void setUpTimeouts();
    static int getConfigTimeoutValue( const std::string& key );
    LuaInterpreterPtr createLuaInterpreter();
    void checkAndLogDefaultTimeout( int defaultTimeout ) const;

    static int getTimeoutSettingValue( CoreDRemoteInterface::CoreDInterface& core,
                                       const std::string& settingName,
                                       int defaultValue );
    void setDefaultTimeout( int timeout );

    void handleSettingsUpdated( const Services::Message& message );
    void updateTimeoutForStoredInterpreters();
    void handleCloudStorageEvents( const std::string& eventType );

    static void printPluginMemoryUsage( int kiloBytes, const std::string& pluginId, const std::string& label );
    static void printProcessMemoryUsage();

private:
    std::unique_ptr< ExtensionsManager::RemoteInterface > m_extensionsManager;

    threads::UniqueSafeWrapper< PluginStatusCallbacksCollection > m_pluginRestartedCallbacks;
    threads::UniqueSafeWrapper< PluginStatusCallbacksCollection > m_pluginTeardownCallbacks;
    threads::UniqueSafeWrapper< PluginStatusCallbacksCollection > m_pluginUninstallCallbacks;

    std::unique_ptr< Luad::TimerManager > m_timerManager;
    int m_scriptTimeout{ DefaultScriptTimeout };
    int m_scriptMinTimeout{ MinScriptTimeout };

    threads::UniqueSafeWrapper< PluginsLuaInterpretersCollection > m_luaInterpreters;
    LuaMetrics m_luaMetrics;
};

} /* namespace LuaAddon */
} /* namespace HubPlatform */
