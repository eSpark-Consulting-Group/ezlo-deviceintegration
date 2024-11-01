#pragma once

#include "networkd/configuration/nm_config_manager.h"

namespace HubPlatform {
namespace Network {

class DummyNetworkConfigManager : public NetworkConfigManager {
public:
    std::shared_ptr< WirelessConfigurationTransaction > startWirelessConfigurationTransaction() override {
        return std::make_shared< WirelessConfigurationTransaction >();
    }
    void prepareEnvironment() override{};

    EthernetConfiguration getEthernetConfiguration( const std::string& interfaceName ) const override {
        return {};
    }
    APWifiConfiguration getAPWifiConfiguration( const std::string& interfaceName ) const override {
        return {};
    }
    ClientWifiConfiguration getClientWifiConfiguration( const std::string& interfaceName ) const override {
        return {};
    }
    MobileConfiguration getMobileConfiguration( const std::string& interfaceName ) const override {
        return {};
    }

    bool configureEthernetInterface( const std::string& interfaceName,
                                     const EthernetConfiguration& ethernetConfiguration ) override {
        return true;
    }
    bool configureAPWifiInterface( const std::string& interfaceName,
                                   const APWifiConfiguration& apWifiConfiguration ) override {
        return true;
    }
    bool configureClientWifiInterface( const std::string& interfaceName,
                                       const ClientWifiConfiguration& clientConfiguration ) override {
        return true;
    }
    bool configureMobileInterface( const std::string& interfaceName,
                                   const MobileConfiguration& mobileConfiguration ) override {
        return true;
    }
};

}  // namespace Network
}  // namespace HubPlatform
