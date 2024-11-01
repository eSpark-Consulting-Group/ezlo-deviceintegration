#include "bluetooth/proxies/advertising_manager_proxy.h"

#include "bluetooth/proxy_method_helpers/register_advertisment_helper.h"
#include "bluetooth/proxy_method_helpers/unregister_advertisment_helper.h"

namespace HubPlatform {

AdvertisingManagerProxy::AdvertisingManagerProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

void AdvertisingManagerProxy::startAdvertising( const std::string& path ) {
    RegisterAdvertismentHelper( *this, path ).callProxyMethod();
}

void AdvertisingManagerProxy::stopAdvertising( const std::string& path ) {
    UnregisterAdvertismentHelper( *this, path ).callProxyMethod();
}

}  // namespace HubPlatform
