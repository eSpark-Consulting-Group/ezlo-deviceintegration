#pragma once

#include "hardware_manager/speaker/environment/speaker_ienvironment.h"

#include <gmock/gmock.h>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

class EnvironmentMock : public IEnvironment {
public:
    MOCK_METHOD( pid_t,
                 fork,
                 ( const std::string& shellCommand, Process::Forker::OnProcessCompletion onProcessCompletion ),
                 ( override ) );

    MOCK_METHOD( void, kill, ( pid_t pid, int signal ), ( override ) );

    MOCK_METHOD( void, sendExecutePluginScript, ( const std::string& script, const Json& scriptParams ), ( override ) );

    MOCK_METHOD( Services::Service&, getService, (), ( override ) );
};

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform