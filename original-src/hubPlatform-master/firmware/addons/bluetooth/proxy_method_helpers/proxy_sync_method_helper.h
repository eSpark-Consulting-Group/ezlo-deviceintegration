#pragma once

#include "bluetooth/proxy_method_helpers/base_proxy_method_helper.h"

#include <future>

namespace HubPlatform {

class ProxySyncMethodHelper : public BaseProxyMethodHelper {
public:
    ProxySyncMethodHelper( Proxy& proxy,
                           std::string method,
                           bool useCustomSetup = false,
                           std::chrono::seconds timeout = std::chrono::seconds( 5 ) );

    void callProxyMethod() override;

protected:
    using BaseProxyMethodHelper::callProxyMethod;

    virtual void parseResult( DBusMessage* message );
    virtual void setup( DBusMessageIter* iter );

private:
    static void returnCallback( DBusMessage* message, void* userData );
    static void setupCallback( DBusMessageIter* iter, void* userData );

    void returnCallback( DBusMessage* message );
    void setupCallback( DBusMessageIter* iter );

private:
    std::promise< void > m_resultSignal;
    std::chrono::seconds m_timeout;
    bool m_useCustomSetup;
};

}  // namespace HubPlatform
