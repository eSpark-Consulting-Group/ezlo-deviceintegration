#include "2gig/2gig_addon.h"
#include "2gig/2gig_dummy_addon.h"
#include "2gig/resources/resources.h"

#include "common_utils/hub_info_helper.h"
#include "common_utils/hub_hardware_info.h"

#include "addon_interfaces/daemon_addon.h"
#include "system_utils/environment.h"

int main( int argc, char* argv[] ) {
    HubHardwareInfo::HubHardwareInfo info( HubInfoHelper::getHubType(), HubInfoHelper::getHardwareRevision() );
    const bool modulePresent = info.get2GIGInfo() == HubHardwareInfo::HubHardwareInfo::Module2GIG::Present;

    if( modulePresent ) {
        HubPlatform::Addons::DaemonAddon< HubPlatform::TwoGig::Addon > daemon(
            HubPlatform::Environment::get2GigDevice(),
            HubPlatform::Environment::getDataFolder() + "/2gig_storage.bin",
            HubPlatform::TwoGig::Resources::ResetPinGPIO );

        return daemon.run( argc, argv );
    }

    HubPlatform::Addons::DaemonAddon< HubPlatform::TwoGig::DummyAddon > daemon;
    return daemon.run( argc, argv );
}
