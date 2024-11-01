#pragma once

#include "bluetooth/proxies/proxy.h"

namespace HubPlatform {

class DeviceProxy final : public Proxy {
public:
    static const char* const ConnectedPropertyName;
    static const char* const PairedPropertyName;

public:
    explicit DeviceProxy( GDBusProxy* proxy );

    std::string getName() const;
    std::string getAddress() const;
    std::string getAdapterPath() const;

    bool isConnected() const;
    bool isPaired() const;

    void connect();
    void disconnect();
    void pair();
};

}  // namespace HubPlatform
