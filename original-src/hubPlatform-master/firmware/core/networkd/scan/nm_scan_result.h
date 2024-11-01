#pragma once

#include "json_utils/json_fwd.h"
#include <string>
#include <map>

namespace HubPlatform {
namespace Network {

namespace NetworkItemAuxFields {
const char* const Band5Ghz = "5G";
const char* const Band2_4Ghz = "2.4G";
}  // namespace NetworkItemAuxFields

struct NetworkItem {
public:
    bool isValid() const;
    Json toJson( bool auxFields ) const;

    bool fromJson( const Json& data, bool auxFields );

    void clear();

public:
    int m_channel = -1;
    std::string m_ssid;
    std::string m_bssid;
    std::string m_security;
    int m_signalPercentLevel = 0;

    struct Aux {
        int m_frequency;
        std::string m_band;
        float m_signal{ 0 };
        bool m_associated{ false };
        unsigned m_capabilitiesFlags{ 0 };
        // TODO: Security: WPA3-personal/PSK2 etc.
    };
    Aux m_aux;
};

using NetworkMap = std::map< std::string /*ssid*/, NetworkItem >;

}  // namespace Network
}  // namespace HubPlatform
