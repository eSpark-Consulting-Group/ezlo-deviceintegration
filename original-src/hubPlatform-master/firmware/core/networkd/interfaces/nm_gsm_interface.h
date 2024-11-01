#pragma once

#include "networkd/gsm/nm_gsm.h"
#include "networkd/gsm/nm_settings_provider.h"
#include "networkd/gsm/nm_gsm_interface_state_manager.h"
#include "networkd/gsm/nm_modem_info_proxy.h"

#include "networkd/interfaces/nm_network_interface.h"
#include "networkd/monitoring/nm_interface_monitor_factory.h"
#include "networkd/monitoring/nm_interface_change_listener.h"
#include "networkd/configuration/nm_config_manager.h"
#include "networkd/interfaces/nm_interface_settings.h"
#include "networkd/configuration/nm_config_file_manager.h"
#include "networkd/monitoring/nm_interface_traffic_monitor.h"

#include "json_utils/json_fwd.h"
#include "provision_data_utils/provision_data.h"
#include "database_client/idbclient.h"
#include "jsonrpc/error_codes.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace Network {

class PingStatistics;

namespace Mobile {
class InterfaceNumber;
}

using namespace GSM;

class GSMInterface final : public InterfaceInfoListener, public NetworkInterface {
    friend class Mobile::InterfaceStateManager;

public:
    GSMInterface( IDbClient& dbClient,
                  const ProvisionDataUtils::ProvisionData& provisionData,
                  NetworkConfigManager& configManager,
                  const InterfaceMonitorFactory& interfaceMonitorFactory,
                  InterfaceInfoListener& interfaceInfoListener,
                  Services::TaskLoop& taskLoop,
                  std::unique_ptr< Mobile::InterfaceNumber > interfaceNumber,
                  std::unique_ptr< GSM::GSMModem > gsmModem,
                  const NetworkConfigFileManager::MobileConfiguration& configuration );

    ~GSMInterface() override;

    void start( const std::weak_ptr< GSMInterface >& weakSelf );

    Json getInfo() const;
    Json getDeviceInfo() const;
    bool isInternetAvailable() const;
    std::string getInterfaceName() const;
    std::string getDeviceName() const;

    void setUserSettings( const std::string& apnName,
                          const std::string& dialNumber,
                          const std::string& pinCode,
                          const std::string& operatorName,
                          const std::string& pdpType );
    void setUserSettingsPinCode( const std::string& pinCode );

    bool isPinCodeRequired() const;
    void enterPinCode( const std::string& pinCode );
    void changePinCode( const std::string& pinCodeNew, const std::string& pinCodeOld );
    void resetPinCode( const std::string& pukCode, const std::string& pinCodeNew );
    void setSIMLock( const std::string& pinCode, bool lock );

    /* @brief establish a PPP connection to a GGSN node (mobile network) */
    void tryUp();

    EnablePolicy getEnablePolicy() const;
    bool setEnablePolicy( EnablePolicy policy );

    /* @brief reset GSM/PPP configurations to default ones */
    void reset();

    void rebootModem();
    void clearFPLMN();
    void reRegister( bool clearFPLMN );

    void pause();
    void restore();

    void onRegistrationEstablished();
    void onRegistrationInterrupted();

    InterfaceTrafficMonitor::TrafficStatistic getTrafficStatistics() const override;
    Json getInstantSignalQuality() const;

    GenericModem& getGenericModem();
    const GenericModem& getGenericModem() const;
    Json getReporterModemInfo() const;

    const PingStatistics& getPingStatistics() const;

private:
    struct GSMModemSettings {
        std::string pinCode;
        std::string apn;
        std::string dialNumber;
    };

    bool enterPinCodeAutomatically();
    bool isUp() const;
    bool scheduleUp();
    void up();
    void down();
    void startMonitoring();
    void stopMonitoring();

    std::string getGSMModemIMEI() const;
    Json getSIMInfo( bool realHwRequest = false ) const;
    bool isSIMInserted() const;
    void captureModemInfo();
    Json getSignalQuality() const;
    GSMModem::AccessTechnology getAccessTechnology() const;
    Json getDebugInformation() const;
    Json getModemInfo() const;

    GSMModemSettings getGSMModemSettings() const;
    Json getGSMModemSettingsJson() const;

    void handleHopelessAttemptToUp();
    Json getInfo( Json dynamicInfo ) const;
    void notifyError( Errors::Error error );

    void onNewInterfaceInfo( Json dynamicInfo ) override;

    const char* getName() const override;
    void onInterfaceActive( bool active ) override;

    MobileConfiguration makeSystemNetworkConfiguration( const GSM::SettingsProvider::Setting& setting ) const;
    void onConnectionEstablished( GSM::SettingsProvider::Setting setting );
    void onConnectionFailed();

private:
    std::unique_ptr< Mobile::InterfaceNumber > m_interfaceNumber;
    std::string m_interfaceName;
    std::string m_deviceName;
    int m_priority;
    int m_metric;
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;

    NetworkConfigManager& m_configManager;
    InterfaceSettings m_settings;
    std::unique_ptr< InterfaceMonitor > m_monitor;
    InterfaceInfoListener& m_interfaceInfoListener;
    Services::TaskLoop& m_taskLoop;
    std::unique_ptr< GSM::GSMModem > m_gsmModem;
    Mobile::InterfaceStateManager m_stateManager;
    Services::TaskLoopTimer m_modemInfoPollingTimer;
    Mobile::ModemInfoProxy m_modemInfoProxy;
    GSM::SettingsProvider m_apnSettingsProvider;
    InterfaceTrafficMonitor m_trafficMonitor;

    std::weak_ptr< GSMInterface > m_weakSelf;

    Errors::Error m_error = Errors::OK;
    bool m_requestedToStop = false;
    bool m_connectionEstablished = false;
    GSM::SettingsProvider::Setting m_successfulSetting;
    bool m_active = false;
    bool m_isFinallyFailed = false;
};

}  // namespace Network
}  // namespace HubPlatform
