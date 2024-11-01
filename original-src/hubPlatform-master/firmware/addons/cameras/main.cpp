#include "cameras/cameras_addon.h"
#include "addon_interfaces/daemon_addon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Addons::DaemonAddon< Cameras::CamerasAddon >().run( argc, argv );
}
