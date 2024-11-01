#pragma once

#include "extensions_manager/common/anonymous_plugin_utils.h"

#include "plugin_utils/plugin_utils.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class FakePluginUtils : public PluginUtils {
public:
    PluginsIds getInstalledPlugins() const override {
        return {};
    }
    PluginsIds getCustomPlugins() const override {
        return {};
    }
    StreamUPtr getInternalPluginLuaScriptFile( const std::string& scriptId ) const override {
        return nullptr;
    }
    StreamUPtr getInternalPluginTemplate( const std::string& ) const override {
        return nullptr;
    }
    StreamUPtr getInternalPluginFile( const std::string& pluginId,
                                      const std::string& internalFilePath ) const override {
        return nullptr;
    }
};

class FakeAnonymousPluginUtils : public IAnonymousPluginUtils {
public:
    PluginId addPlugin( const std::string& scriptCode, const std::string& scriptName ) override {
        return "";
    }
    AnonymousPluginError changePlugin( const std::string& pluginId,
                                       const std::string& scriptCode,
                                       const std::string& scriptName ) override {
        return {};
    }
    AnonymousPlugin getPlugin( const std::string& pluginId ) const override {
        return {};
    }
    AnonymousPlugins listPlugins() const override {
        return {};
    }
    AnonymousPluginError deletePlugin( const std::string& pluginId ) override {
        return {};
    }
    AnonymousPluginError runPlugin( const std::string& pluginId ) override {
        return {};
    }
    std::string getPluginScriptContent( const std::string& pluginId ) const override {
        return "";
    }
    bool checkPluginCode( const std::string& scriptCode ) const override {
        return true;
    }
    bool checkPluginName( const std::string& scriptName ) const override {
        return true;
    }
    bool pluginExists( const std::string& pluginId ) const override {
        return true;
    }
    bool isPluginAcquired( const std::string& pluginId ) override {
        return true;
    }
    AnonymousPluginError acquirePlugin( const std::string& pluginId, bool ) override {
        return {};
    }
    void releasePlugin( const std::string& pluginId ) override {}
    ScriptId getScriptIdByPluginId( const std::string& pluginId ) const override {
        return "";
    }
    PluginId getPluginIdByScriptId( const std::string& scriptId ) const override {
        return "";
    }
    bool isPluginScript( const std::string& scriptId ) const override {
        return false;
    }
    ScriptStreamUPtr getLuaScript( const std::string& scriptId ) const override {
        return nullptr;
    }
    std::string getLuaScriptFilePath( const std::string& scriptId ) const override {
        return "";
    }
    std::string getDataFilePath( const std::string& scriptId ) const override {
        return "";
    }
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
