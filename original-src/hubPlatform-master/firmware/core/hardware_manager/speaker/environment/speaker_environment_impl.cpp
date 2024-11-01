
#include "hardware_manager/speaker/environment/speaker_environment_impl.h"
#include "hardware_manager/hm_service.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

EnvironmentImpl::EnvironmentImpl( Service& service )
: m_service( service )
, m_forker( Process::Forker::getInstance() ) {}

pid_t EnvironmentImpl::fork( const std::string& shellCommand,
                             Process::Forker::OnProcessCompletion onProcessCompletion ) {
    return m_forker->fork( shellCommand, std::move( onProcessCompletion ) );
}

void EnvironmentImpl::kill( pid_t pid, int signal ) {
    m_forker->kill( pid, signal );
}

void EnvironmentImpl::sendExecutePluginScript( const std::string& script, const Json& scriptParams ) {
    m_service.sendExecutePluginScript( script, scriptParams );
}

Services::Service& EnvironmentImpl::getService() {
    return m_service;
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
