#include "zwave_addon.h"

#include "addon_interfaces/daemon_addon.h"
#include "system_utils/environment.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    // TODO: Investigate how to pass default params to addon
    return Addons::DaemonAddon< Zwave::ZwaveAddon >( Environment::getZwaveDevice(),
                                                     Environment::getDataFolder() + "/zwstore.bin" )
        .run( argc, argv );
}
