#include "lua_metrics.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace LuaAddon {

void MetricsData::interpreterCreated() {
    m_interpreterCreated++;
}

void MetricsData::scriptStarted() {
    m_scriptStarted++;
}

void MetricsData::scriptFinished( Duration duration ) {
    m_scriptFinished++;
    m_duration += duration;
}

void MetricsData::scriptFailed( Duration duration ) {
    m_scriptFailed++;
    m_duration += duration;
}

void MetricsData::scriptAborted( Duration duration ) {
    m_scriptAborted++;
    m_duration += duration;
}

void MetricsData::scriptTimeout( Duration duration ) {
    m_scriptTimeout++;
    m_duration += duration;
}

void MetricsData::pluginMemoryUsed( size_t kiloBytes ) {
    m_memoryUsed = kiloBytes;
    m_policy = Policy::RESTORE_STATE;
}

namespace MetricsFields {
const char* const InterpreterCreated = "interpreterCreated";
const char* const ScriptStarted = "scriptStarted";
const char* const ScriptFinished = "scriptFinished";
const char* const ScriptAborted = "scriptAborted";
const char* const ScriptFailed = "scriptFailed";
const char* const ScriptTimeout = "scriptTimeout";
const char* const Duration = "duration";
const char* const Policy = "policy";
const char* const MemoryUsageKb = "memoryUsageKb";
}  // namespace MetricsFields

Json MetricsData::toJson() const {
    using namespace MetricsFields;
    Json result = { { InterpreterCreated, m_interpreterCreated },
                    { ScriptStarted, m_scriptStarted },
                    { ScriptFinished, m_scriptFinished },
                    { ScriptFailed, m_scriptFailed },
                    { ScriptAborted, m_scriptAborted },
                    { ScriptTimeout, m_scriptTimeout },
                    { MetricsFields::Duration,
                      cc::duration2string( std::chrono::duration_cast< std::chrono::nanoseconds >( m_duration ) ) } };
    if( m_policy == Policy::RESTORE_STATE ) {
        result[ MetricsFields::Policy ] = "restoreState";
        result[ MetricsFields::MemoryUsageKb ] = m_memoryUsed;
    }
    else {
        result[ MetricsFields::Policy ] = "newState";
    }
    return result;
}

void LuaMetrics::onInterpreterCreated( const std::string& pluginId ) {
    m_globalData.interpreterCreated();
    m_pluginData[ pluginId ].interpreterCreated();
}

void LuaMetrics::onScriptStarted( const std::string& pluginId, const std::string& /* script */ ) {
    m_globalData.scriptStarted();
    m_pluginData[ pluginId ].scriptStarted();
}

void LuaMetrics::onScriptFinished( const std::string& pluginId, const std::string& /* script */, Duration duration ) {
    m_globalData.scriptFinished( duration );
    m_pluginData[ pluginId ].scriptFinished( duration );
}

void LuaMetrics::onScriptFailed( const std::string& pluginId, const std::string& /* script */, Duration duration ) {
    m_globalData.scriptFailed( duration );
    m_pluginData[ pluginId ].scriptFailed( duration );
}

void LuaMetrics::onScriptAborted( const std::string& pluginId, const std::string& /* script */, Duration duration ) {
    m_globalData.scriptAborted( duration );
    m_pluginData[ pluginId ].scriptAborted( duration );
}

void LuaMetrics::onScriptTimeout( const std::string& pluginId, const std::string& /* script */, Duration duration ) {
    m_globalData.scriptTimeout( duration );
    m_pluginData[ pluginId ].scriptTimeout( duration );
}

void LuaMetrics::onPluginResourceUsage( const std::string& pluginId, const std::string& policy, int kiloBytes ) {
    m_pluginData[ pluginId ].pluginMemoryUsed( kiloBytes );
}

Json LuaMetrics::toJson() {
    Json plugins = JSON_EMPTY_OBJ;
    for( auto it = m_pluginData.begin(); it != m_pluginData.end(); it++ ) {
        auto& name = it->first;
        auto& metrics = it->second;

        plugins[ name ] = metrics.toJson();
    }

    Json result = { { "global", m_globalData.toJson() }, { "plugins", plugins } };
    return result;
}

}  // namespace LuaAddon
}  // namespace HubPlatform
