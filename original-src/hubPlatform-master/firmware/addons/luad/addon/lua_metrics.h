#pragma once
#include <string>
#include <map>
#include <chrono>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace LuaAddon {

using Duration = std::chrono::steady_clock::duration;

struct MetricsData {
public:
    void interpreterCreated();
    void scriptStarted();
    void scriptFinished( Duration duration );
    void scriptFailed( Duration duration );
    void scriptAborted( Duration duration );
    void scriptTimeout( Duration duration );
    void pluginMemoryUsed( size_t kiloBytes );

    Json toJson() const;

private:
    size_t m_interpreterCreated{ 0 };
    size_t m_scriptStarted{ 0 };
    size_t m_scriptFinished{ 0 };
    size_t m_scriptFailed{ 0 };
    size_t m_scriptAborted{ 0 };
    size_t m_scriptTimeout{ 0 };
    Duration m_duration{ 0 };

    enum class Policy : int { NEW_STATE = 1, RESTORE_STATE = 2 };
    Policy m_policy{ Policy::NEW_STATE };
    size_t m_memoryUsed{ 0 };
};

class LuaMetrics {
public:
    void onInterpreterCreated( const std::string& pluginId );
    void onScriptStarted( const std::string& pluginId, const std::string& script );
    void onScriptFinished( const std::string& pluginId, const std::string& script, Duration duration );
    void onScriptFailed( const std::string& pluginId, const std::string& script, Duration duration );
    void onScriptAborted( const std::string& pluginId, const std::string& script, Duration duration );
    void onScriptTimeout( const std::string& pluginId, const std::string& script, Duration duration );
    void onPluginResourceUsage( const std::string& pluginId, const std::string& policy, int kiloBytes );

    Json toJson();

private:
    MetricsData m_globalData;
    std::map< std::string, MetricsData > m_pluginData;
    // TODO: m_scriptData
};

}  // namespace LuaAddon
}  // namespace HubPlatform
