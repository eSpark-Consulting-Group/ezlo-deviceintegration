#include "bluetooth/proxy_method_helpers/base_proxy_method_helper.h"

namespace HubPlatform {

BaseProxyMethodHelper::BaseProxyMethodHelper( Proxy& proxy, std::string method )
: m_proxy( proxy )
, m_method( std::move( method ) ) {}

BaseProxyMethodHelper::~BaseProxyMethodHelper() = default;

bool BaseProxyMethodHelper::callProxyMethod( void* userData,
                                             GDBusSetupFunction setupFunc,
                                             GDBusReturnFunction returnFunc ) {
    return g_dbus_proxy_method_call(
               m_proxy.getProxyPtr(), m_method.c_str(), setupFunc, returnFunc, userData, nullptr ) == TRUE;
}

const std::string& BaseProxyMethodHelper::getMethod() const {
    return m_method;
}

}  // namespace HubPlatform
