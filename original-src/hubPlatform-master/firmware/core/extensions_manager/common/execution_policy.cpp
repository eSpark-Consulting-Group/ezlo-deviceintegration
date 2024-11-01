
#include "extensions_manager/common/execution_policy.h"
#include "extensions_manager/resources/em_resources.h"

#include <map>

namespace HubPlatform {
namespace ExtensionsManager {

ExecutionPolicy toExecutionPolicy( const std::string& executionPolicyString ) {
    auto type = ExecutionPolicy::Unknown;

    static const std::map< std::string, ExecutionPolicy > typeOfPolicy = {
        { Resources::ExecutionPolicyStates::NewStateEachScriptRun, ExecutionPolicy::NewStateEachScriptRun },
        { Resources::ExecutionPolicyStates::RestoreLastScriptState, ExecutionPolicy::RestoreLastScriptState },
        { Resources::ExecutionPolicyStates::Unknown, ExecutionPolicy::Unknown }
    };

    auto it = typeOfPolicy.find( executionPolicyString );
    if( it != typeOfPolicy.end() ) {
        type = it->second;
    }

    return type;
}

std::string toString( ExecutionPolicy executionPolicy ) {
    switch( executionPolicy ) {
    case ExecutionPolicy::NewStateEachScriptRun:
        return Resources::ExecutionPolicyStates::NewStateEachScriptRun;

    case ExecutionPolicy::RestoreLastScriptState:
        return Resources::ExecutionPolicyStates::RestoreLastScriptState;

    case ExecutionPolicy::Unknown:
    default:
        return Resources::ExecutionPolicyStates::Unknown;
    }

    return Resources::ExecutionPolicyStates::Unknown;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
