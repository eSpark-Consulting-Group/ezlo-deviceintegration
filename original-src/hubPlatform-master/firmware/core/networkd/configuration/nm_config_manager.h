#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace Network {

struct ModeConfiguration {
    Json toJson() const;

    std::string m_mode;
    std::string m_ip;
    std::string m_netMask;
};

struct EthernetConfiguration {
    bool isEmtpy() const;
    Json toJson() const;

    ModeConfiguration m_mode;
};

struct APWifiConfiguration {
    bool isEmtpy() const;
    Json toJson() const;

    std::string m_ssid;
    std::string m_encryption;
    std::string m_key;
    int m_channel = 0;
    int m_ipLimit = 0;

    ModeConfiguration m_mode;
};

struct ClientWifiConfiguration {
    bool isEmtpy() const;
    Json toJson() const;

    std::string m_ssid;
    std::string m_encryption;
    std::string m_key;
    int m_channel = 0;
    std::string m_bssid;

    ModeConfiguration m_mode;
};

struct MobileConfiguration {
    bool isEmtpy() const;
    Json toJson() const;

    std::string m_apn;
    std::string m_device;
    std::string m_pppName;
    std::string m_dialNumber;
    bool m_ipv6 = false;

    std::string m_mode;
    std::string m_metric;
};

class NetworkConfigManager {
public:
    class WirelessConfigurationTransaction {
    public:
        virtual ~WirelessConfigurationTransaction() = default;
        virtual void setModified( bool hard = false ){};
    };

public:
    virtual ~NetworkConfigManager() = default;

    virtual std::shared_ptr< WirelessConfigurationTransaction > startWirelessConfigurationTransaction() = 0;
    virtual void prepareEnvironment() = 0;

    virtual EthernetConfiguration getEthernetConfiguration( const std::string& interfaceName ) const = 0;
    virtual APWifiConfiguration getAPWifiConfiguration( const std::string& interfaceName ) const = 0;
    virtual ClientWifiConfiguration getClientWifiConfiguration( const std::string& interfaceName ) const = 0;
    virtual MobileConfiguration getMobileConfiguration( const std::string& interfaceName ) const = 0;

    virtual bool configureEthernetInterface( const std::string& interfaceName,
                                             const EthernetConfiguration& configuration = {} ) = 0;
    virtual bool configureAPWifiInterface( const std::string& interfaceName,
                                           const APWifiConfiguration& configuration = {} ) = 0;
    virtual bool configureClientWifiInterface( const std::string& interfaceName,
                                               const ClientWifiConfiguration& configuration = {} ) = 0;
    virtual bool configureMobileInterface( const std::string& interfaceName,
                                           const MobileConfiguration& configuration = {} ) = 0;
};

}  // namespace Network
}  // namespace HubPlatform
