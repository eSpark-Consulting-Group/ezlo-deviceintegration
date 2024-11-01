#pragma once

#include "networkd/gsm/nm_gsm.h"
#include "networkd/configuration/nm_config_file_manager.h"

#include "database_client/idbclient.h"
#include "provision_data_utils/provision_data.h"

#include <string>
#include <deque>

namespace HubPlatform {
namespace Network {
namespace GSM {

class SettingsProvider final {
public:
    struct Setting {
        Setting() = default;
        Setting( std::string apnName,
                 std::string dialNumber,
                 std::string pinCode,
                 std::string operatorName,
                 std::string pdpType );

        explicit Setting( const Json& json );

        operator bool() const;
        bool operator!() const;
        Json toJson() const;

        std::string m_apnName;
        std::string m_dialNumber;
        std::string m_pinCode;
        std::string m_operatorName;
        std::string m_pdpType;
    };

    using APNNames = std::vector< std::string >;

    SettingsProvider( std::string interfaceName,
                      IDbClient& dbClient,
                      GSMModem& gsmModem,
                      const ProvisionDataUtils::ProvisionData& provisionData,
                      const NetworkConfigFileManager::MobileConfiguration& configuration );
    void init();

    void reconstructSettings();
    Setting getNextSetting();
    void selectFirstSettingsProfile();

    Setting getDatabaseUserSetting() const;
    void setDatabaseUserSetting( const Setting& setting );
    void resetDatabaseUserSetting();

private:
    Setting combineSettings( const Setting& baseSetting, const Setting& auxSetting ) const;
    Setting getCurrentSetting() const;
    Setting getTemplateSetting( const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const;
    void constructProposalSettings();
    void showProposalSettings() const;
    std::string getUserFriendlyOperatorName( const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const;

    APNNames getProposalAPNNames( const GSMModem::SIMInfo& simInfo,
                                  const GSMModem::MobileNetworkInfo& mobileNetworkInfo ) const;

private:
    const std::string m_interfaceName;
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;
    IDbClient& m_dbClient;
    GSMModem& m_gsmModem;
    const ProvisionDataUtils::ProvisionData& m_provisionData;

    std::deque< Setting > m_settings;
    std::size_t m_currentSetting = 0;
    bool m_hasUserSettings = false;
    std::string m_simICCID;
    std::size_t m_simSettingsStorageSize = 4;
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
