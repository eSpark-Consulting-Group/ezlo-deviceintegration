#pragma once

#include "bluetooth/proxies/proxy.h"

namespace HubPlatform {

class AdapterProxy : public Proxy {
public:
    static const char* const DiscoveringPropertyName;

public:
    explicit AdapterProxy( GDBusProxy* proxy );

    void setPowered( bool powered = true );
    void setDiscoverable( bool discoverable = true );

    void startDiscovery();
    void stopDiscovery();

    bool isDiscovering() const;

    std::string getAddress() const;

    void removeDevice( const std::string& devicePath );
};

}  // namespace HubPlatform
