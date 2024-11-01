#include "bluetooth/proxies/adapter_proxy.h"

#include "bluetooth/proxy_method_helpers/start_discovery_helper.h"
#include "bluetooth/proxy_method_helpers/stop_discovery_helper.h"
#include "bluetooth/proxy_method_helpers/remove_device_helper.h"

namespace HubPlatform {

const char* const AdapterProxy::DiscoveringPropertyName = "Discovering";

AdapterProxy::AdapterProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

void AdapterProxy::setPowered( bool powered ) {
    setBooleanProperty( "Powered", powered );
}

void AdapterProxy::setDiscoverable( bool discoverable ) {
    setBooleanProperty( "Discoverable", discoverable );
}

void AdapterProxy::startDiscovery() {
    StartDiscoveryHelper( *this ).callProxyMethod();
}

void AdapterProxy::stopDiscovery() {
    StopDiscoveryHelper( *this ).callProxyMethod();
}

std::string AdapterProxy::getAddress() const {
    return getProperty< std::string >( "Address" );
}

void AdapterProxy::removeDevice( const std::string& devicePath ) {
    RemoveDeviceHelper( *this, devicePath ).callProxyMethod();
}

bool AdapterProxy::isDiscovering() const {
    return getProperty< bool >( DiscoveringPropertyName );
}

}  // namespace HubPlatform
