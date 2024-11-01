#include "bluetooth/bluetooth_addon.h"

#include "addon_interfaces/daemon_addon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Addons::DaemonAddon< BluetoothAddon >().run( argc, argv );
}
