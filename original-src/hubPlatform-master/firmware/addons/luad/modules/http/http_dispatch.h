#pragma once

#include "http/http.h"

namespace HubPlatform {

class LuaPluginResourcesProvider;

namespace LuaAddon {
class EventEmitter;
}

namespace Luad {

using namespace HubPlatform::CommonHttp;

class IPluginStatusEventsSubscription;

class HttpDispatch {
public:
    HttpDispatch( LuaPluginResourcesProvider& luaPluginResourceProvider,
                  LuaAddon::EventEmitter& eventsEmitter,
                  IPluginStatusEventsSubscription& pluginStatusEventsSubscription );
    ~HttpDispatch();

    std::string request( const std::string& pluginId, const Json& request );
    void repeatRequest( const std::string& pluginId, const Json& request );
    bool closeConnection( const std::string& pluginId, const std::string& request );
    void closePluginConnections( const std::string& pluginId );
    int sendData( const std::string& pluginId, Json request );
    Json getConnectionInfo( const std::string& pluginId, const std::string& connectionId );

private:
    void checkPluginPermissions( const std::string& pluginId, const std::string& script );
    static void checkFullPathExists( const std::string& script, const std::string& nameForException );
    std::string makeScriptFullPath( const std::string& script );
    std::string makeDataFileFullPath( const std::string& fileName );

    std::string checkScriptFileAndMakeFullPath( const std::string& pluginId, const std::string& script );
    std::string checkDataFileAndMakeFullPath( const std::string& pluginId, const std::string& fileName );

private:
    LuaPluginResourcesProvider& m_luaPluginResourceProvider;
    LuaAddon::EventEmitter& m_eventsEmitter;
    Http m_http;
};

}  // namespace Luad
}  // namespace HubPlatform
