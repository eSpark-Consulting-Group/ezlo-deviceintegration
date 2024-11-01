#include "bluetooth/proxies/proxy.h"

#include "bluetooth/log.h"
#include "bluetooth/exceptions/bad_property_type.h"

namespace HubPlatform {

Proxy::Proxy( GDBusProxy* proxy )
: m_proxy( proxy ) {}

Proxy::~Proxy() = default;

GDBusProxy* Proxy::getProxyPtr() const {
    return m_proxy;
}

std::string Proxy::getProxyPath() const {
    return g_dbus_proxy_get_path( m_proxy );
}

std::string Proxy::getInterface() const {
    return g_dbus_proxy_get_interface( m_proxy );
}

bool operator==( const GDBusProxy* proxyPtr, const Proxy& proxyObj ) {
    return proxyPtr == proxyObj.getProxyPtr();
}

bool operator==( const Proxy& proxyObj, const GDBusProxy* proxyPtr ) {
    return proxyPtr == proxyObj.getProxyPtr();
}

}  // namespace HubPlatform
