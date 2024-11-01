#include "mqtt_gateway/mqtt_gateway_addon.h"
#include "addon_interfaces/daemon_addon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Addons::DaemonAddon< MQTTGatewayAddon >().run( argc, argv );
}
