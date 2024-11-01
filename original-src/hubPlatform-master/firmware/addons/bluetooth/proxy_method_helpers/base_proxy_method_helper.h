#pragma once

#include "bluetooth/proxies/proxy.h"

#include <gdbus.h>

namespace HubPlatform {

class BaseProxyMethodHelper {
public:
    BaseProxyMethodHelper( Proxy& proxy, std::string method );
    virtual ~BaseProxyMethodHelper();

    virtual void callProxyMethod() = 0;

protected:
    const std::string& getMethod() const;

    bool callProxyMethod( void* userData, GDBusSetupFunction setupFunc, GDBusReturnFunction returnFunc );

private:
    Proxy& m_proxy;
    std::string m_method;
};

}  // namespace HubPlatform
