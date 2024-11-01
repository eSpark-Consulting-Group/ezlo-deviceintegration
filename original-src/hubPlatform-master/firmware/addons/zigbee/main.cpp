#include "addon_interfaces/daemon_addon.h"
#include "zigbee/zigbee_addon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Addons::DaemonAddon< Zigbee::ZigbeeAddon >().run( argc, argv );
}
