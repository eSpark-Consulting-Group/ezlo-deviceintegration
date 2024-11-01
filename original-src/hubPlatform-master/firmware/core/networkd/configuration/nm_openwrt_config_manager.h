#pragma once

#include "networkd/configuration/nm_config_manager.h"

#include "database_client/idbclient.h"
#include "provision_data_utils/provision_data.h"
#include "uci_utils/uci_wireless_file.h"
#include "uci_utils/uci_network_file.h"

namespace HubPlatform {
namespace Network {

class OpenwrtNetworkConfigManager final : public NetworkConfigManager {
public:
    OpenwrtNetworkConfigManager( const ProvisionDataUtils::ProvisionData& provisionData, IDbClient& dbClient );

    std::shared_ptr< WirelessConfigurationTransaction > startWirelessConfigurationTransaction() override;
    void prepareEnvironment() override;

    EthernetConfiguration getEthernetConfiguration( const std::string& interfaceName ) const override;
    APWifiConfiguration getAPWifiConfiguration( const std::string& interfaceName ) const override;
    ClientWifiConfiguration getClientWifiConfiguration( const std::string& interfaceName ) const override;
    MobileConfiguration getMobileConfiguration( const std::string& interfaceName ) const override;

    bool configureEthernetInterface( const std::string& interfaceName,
                                     const EthernetConfiguration& configuration = {} ) override;
    bool configureAPWifiInterface( const std::string& interfaceName,
                                   const APWifiConfiguration& configuration = {} ) override;
    bool configureClientWifiInterface( const std::string& interfaceName,
                                       const ClientWifiConfiguration& configuration = {} ) override;
    bool configureMobileInterface( const std::string& interfaceName,
                                   const MobileConfiguration& configuration = {} ) override;

private:
    bool fixWirelessConfig();

    bool isWirelessConfigured( const std::string& interfaceName ) const;

    Json getSystemWirelessConfig( const std::string& interfaceName ) const;
    Json getSystemNetworkConfig( const std::string& interfaceName ) const;

    bool setSystemWirelessConfig( const char* interfaceName, const Json& config );
    bool resetSystemWirelessConfig( const char* interfaceName, bool hard = false );

    bool hasSystemNetworkConfigSection( const char* interfaceName );
    bool setSystemNetworkConfigSection( UciUtils::UciSection section, bool reload = false );
    bool removeSystemNetworkConfigSection( const char* interfaceName, bool reload = false );

    ModeConfiguration getModeConfiguration( const std::string& interfaceName ) const;

    bool eraseDisabledOptions( UciUtils::UciSection& section );

private:
    UciUtils::UciWirelessFile m_wirelessFile;
    UciUtils::UciNetworkFile m_networkFile;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    std::weak_ptr< WirelessConfigurationTransaction > m_wirelessConfigurationTransaction;
    IDbClient& m_dbClient;

    APWifiConfiguration m_apWifiConfiguration;
};

}  // namespace Network
}  // namespace HubPlatform
