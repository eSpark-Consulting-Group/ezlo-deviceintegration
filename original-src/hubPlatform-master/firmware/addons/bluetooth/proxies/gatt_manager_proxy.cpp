#include "bluetooth/proxies/gatt_manager_proxy.h"

#include "bluetooth/proxy_method_helpers/register_gatt_application_helper.h"

namespace HubPlatform {

GattManagerProxy::GattManagerProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

void GattManagerProxy::registerApplication( const std::string& path ) {
    RegisterGattApplicationHelper( *this, path ).callProxyMethod();
}

}  // namespace HubPlatform
