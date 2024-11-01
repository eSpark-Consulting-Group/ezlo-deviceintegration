#pragma once

#include "bluetooth/proxy_method_helpers/base_proxy_method_helper.h"

namespace HubPlatform {

class ProxyAsyncMethodHelper : public BaseProxyMethodHelper {
public:
    ProxyAsyncMethodHelper( Proxy& proxy, std::string method, bool useCustomSetup = false );

    void callProxyMethod() override;

protected:
    using BaseProxyMethodHelper::callProxyMethod;

    virtual void setup( DBusMessageIter* iter );

private:
    static void setupCallback( DBusMessageIter* iter, void* userData );
    void setupCallback( DBusMessageIter* iter );

private:
    bool m_useCustomSetup;
};

}  // namespace HubPlatform
