#include "vidoo4hub/vidoo4hub_addon.h"
#include "addon_interfaces/daemon_addon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Addons::DaemonAddon< Vidoo4HubAddon >().run( argc, argv );
}
