#pragma once

#include <string>

namespace HubPlatform {
namespace ExtensionsManager {

enum class ExecutionPolicy {
    Unknown,

    NewStateEachScriptRun,
    RestoreLastScriptState
};

ExecutionPolicy toExecutionPolicy( const std::string& executionPolicyString );

std::string toString( ExecutionPolicy policy );

}  // namespace ExtensionsManager
}  // namespace HubPlatform
