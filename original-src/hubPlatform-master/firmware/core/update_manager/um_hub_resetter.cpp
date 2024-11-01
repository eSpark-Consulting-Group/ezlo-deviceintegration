#include "update_manager/um_hub_resetter.h"

#include "hardware_manager/remote_interface/hm_remote_interface.h"

#include "system_utils/environment.h"
#include "common_utils/lock_helper.h"
#include "log_utils/ezlo_log.h"

#include <thread>

namespace HubPlatform {
namespace UpdateManager {

namespace {
constexpr const std::chrono::seconds ResetDelay( 1 );
constexpr const std::chrono::seconds ResetTimeout( 20 );
}  // namespace

HubResetter::HubResetter( HardwareManager::RemoteInterface& hardwareManager )
: m_hardwareManager( hardwareManager ) {}

void HubResetter::reset( HubResetType resetType ) {
    switch( resetType ) {
    case HubResetType::Factory:
        handleFactoryReset();
        break;
    case HubResetType::Soft:
        handleSoftReset();
        break;
    default:
        Log::error( "Attempt to perform reset of unknown type" );
        break;
    }
}

void HubResetter::switchToLuup() {
    using namespace HubPlatform::HardwareManager;
    Log::debug( "Request switch to Luup firmware" );

    m_hardwareManager.onProcessRunning( HubState::Process::SwitchFirmwareToLuup );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon --switch_to_luup" ).c_str() );

    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
    m_hardwareManager.onProcessStopped( HubState::Process::SwitchFirmwareToLuup, HubState::ProcessResult::Fail );
}

void HubResetter::handleFactoryReset() {
    using namespace HubPlatform::HardwareManager;
    Log::debug( "Request factory reset" );

    m_hardwareManager.onProcessRunning( HubState::Process::FactoryReset );
    std::this_thread::sleep_for( ResetDelay );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon --factory_reset" ).c_str() );

    std::this_thread::sleep_for( ResetTimeout );
    m_hardwareManager.onProcessStopped( HubState::Process::FactoryReset, HubState::ProcessResult::Fail );
}

void HubResetter::handleSoftReset() {
    using namespace HubPlatform::HardwareManager;
    Log::debug( "Request soft reset" );

    m_hardwareManager.onProcessRunning( HubState::Process::SoftReset );
    std::this_thread::sleep_for( ResetDelay );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon --soft_reset" ).c_str() );

    std::this_thread::sleep_for( ResetTimeout );
    m_hardwareManager.onProcessStopped( HubState::Process::SoftReset, HubState::ProcessResult::Fail );
}

}  // namespace UpdateManager
}  // namespace HubPlatform
