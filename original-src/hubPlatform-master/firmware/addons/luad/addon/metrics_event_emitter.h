#pragma once
#include <string>
#include <chrono>
#include "base_utils/event_emitter.h"

namespace HubPlatform {
namespace LuaAddon {

class MetricsEventEmitter {
public:
    MetricsEventEmitter() = default;
    using Duration = std::chrono::steady_clock::duration;

public:
    virtual void emitInterpreterCreated( std::string pluginId ) = 0;
    virtual void emitScriptStarted( std::string plugin, std::string script, std::string operationId ) = 0;
    virtual void emitScriptFinished( std::string plugin,
                                     std::string script,
                                     std::string operationId,
                                     Duration duration ) = 0;
    virtual void emitScriptFailed( std::string plugin,
                                   std::string script,
                                   std::string operationId,
                                   Duration duration ) = 0;
    virtual void emitScriptTimeout( std::string plugin,
                                    std::string script,
                                    std::string operationId,
                                    Duration duration ) = 0;
    virtual void emitScriptAborted( std::string plugin, std::string script, std::string operationId ) = 0;
    virtual void emitPluginResourceUsage( std::string pluginId, std::string policy, int kiloBytes ) = 0;
};

}  // namespace LuaAddon
}  // namespace HubPlatform