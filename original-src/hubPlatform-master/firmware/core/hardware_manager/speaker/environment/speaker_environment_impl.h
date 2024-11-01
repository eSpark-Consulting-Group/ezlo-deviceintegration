#pragma once

#include "hardware_manager/speaker/environment/speaker_ienvironment.h"

namespace HubPlatform {
namespace HardwareManager {

class Service;

namespace Speaker {

class EnvironmentImpl : public IEnvironment {
public:
    EnvironmentImpl( Service& service );

public:
    pid_t fork( const std::string& shellCommand, Process::Forker::OnProcessCompletion onProcessCompletion ) override;

    void kill( pid_t pid, int signal ) override;

    void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) override;

    Services::Service& getService() override;

private:
    Service& m_service;

    Process::Forker::Instance m_forker;
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
