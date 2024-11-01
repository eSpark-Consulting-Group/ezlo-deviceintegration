#include "bluetooth/proxies/device_proxy.h"

#include "bluetooth/proxy_method_helpers/connect_helper.h"
#include "bluetooth/proxy_method_helpers/disconnect_helper.h"
#include "bluetooth/proxy_method_helpers/pair_helper.h"

namespace HubPlatform {

const char* const DeviceProxy::ConnectedPropertyName = "Connected";
const char* const DeviceProxy::PairedPropertyName = "Paired";

DeviceProxy::DeviceProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

std::string DeviceProxy::getAdapterPath() const {
    return getProperty< std::string >( "Adapter" );
}

bool DeviceProxy::isConnected() const {
    return getProperty< bool >( ConnectedPropertyName );
}

bool DeviceProxy::isPaired() const {
    return getProperty< bool >( PairedPropertyName );
}

std::string DeviceProxy::getName() const {
    const auto name = getProperty< std::string >( "Alias" );
    return !name.empty() ? name : "<unknown>";
}

std::string DeviceProxy::getAddress() const {
    const auto address = getProperty< std::string >( "Address" );
    return !address.empty() ? address : "<unknown>";
}

void DeviceProxy::connect() {
    ConnectHelper( *this ).callProxyMethod();
}

void DeviceProxy::disconnect() {
    DisconnectHelper( *this ).callProxyMethod();
}

void DeviceProxy::pair() {
    PairHelper( *this ).callProxyMethod();
}

}  // namespace HubPlatform
