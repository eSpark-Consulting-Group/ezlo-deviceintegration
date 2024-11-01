#pragma once

#include "system_utils/process/forker.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Services {
class Service;
}  // namespace Services
namespace HardwareManager {
namespace Speaker {

class IEnvironment {
public:
    virtual ~IEnvironment() = default;

public:
    virtual pid_t fork( const std::string& shellCommand, Process::Forker::OnProcessCompletion onProcessCompletion ) = 0;

    virtual void kill( pid_t pid, int signal ) = 0;

    virtual void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) = 0;

    virtual Services::Service& getService() = 0;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
