#pragma once

#include "networkd/remote_interface/network_statistics_settings_manager.h"

#include "json_utils/json_fwd.h"

#include <chrono>
#include <string>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace Network {

class RemoteInterface {
public:
    struct MobileSettings {
        MobileSettings();

        std::string m_operatorName;
        std::string m_apn;
        std::string m_dialNumber;
        std::string m_pinCode;
        std::string m_pdpType;

        static MobileSettings constructMobileSettings( const Json& json );
        Json toJson() const;
    };

    class StatisticsPeriod {
    public:
        StatisticsPeriod();
        explicit StatisticsPeriod( const Json& period );

        Json toJson() const;
        bool isValid() const;

        const std::string& begin() const;
        const std::string& end() const;

    private:
        std::string m_begin;  // yyyy-mm-dd
        std::string m_end;    // yyyy-mm-dd
        bool m_isValid = false;
    };

    explicit RemoteInterface( Services::Service& hostService );

    bool isInterfaceAllowedToReset( const std::string& interfaceId ) const;
    bool isGSMInterface( const std::string& interfaceId ) const;
    bool isInterfaceExist( const std::string& interfaceId ) const;

    std::vector< Json > getNetworkInterfaces();
    std::vector< Json > getConnectedNetworkInterfaces();
    Json getActiveNetworkInterface();
    Json getLocalNetworkInterface();
    bool isInternetAvailable() const;
    void resetNetwork( const std::string& interfaceId );
    void wifiScanStart( const std::string& interfaceId );
    void wifiScanStop( const std::string& interfaceId );
    void wifiTryConnect( const std::string& ssid,
                         const std::string& bssid,
                         const std::string& encryption,
                         const std::string& key,
                         bool force );
    void modemTryConnect( const std::string& interfaceId, const MobileSettings& mobileSettings );
    void modemPinCodeSet( const std::string& interfaceId,
                          const std::string& pinCodeOld,
                          const std::string& pinCodeNew );
    void modemPinCodeReset( const std::string& interfaceId, const std::string& pukCode, const std::string& pinCodeNew );
    void modemSIMLockSet( const std::string& interfaceId, const std::string& pinCode, bool lock );
    void modemClearFPLMN( const std::string& interfaceId );
    void modemReRegister( const std::string& interfaceId, bool clearFPLM );

    StatisticsSettingsManager::StatisticsSettings getStatisticsSettings();
    Json setStatisticsSettings( const StatisticsSettingsManager::StatisticsSettings& settings );
    void getStatistics( const StatisticsPeriod& period );

    Json setEnablePolicy( const std::string& interfaceId, const std::string& enablePolicy );
    Json setCustomSettings( const std::string& interfaceId, const Json& settings );

    void sshTunnelSupportEnabledSet( bool enabled, int endTime );
    Json getSshTunnelSupportInfo();

    /* modem provides adc functionality (port 0 and port 1);
     * networkd should have exclusive control over a modem */
    using GetADCValueCallback = std::function< void( float ) >;
    void requestADCValue( GetADCValueCallback cb, int adcPort = 0 );
    void communicationTest( const std::string& interface,
                            const std::string& operationId,
                            std::chrono::seconds timeout,
                            const std::function< void( const JsonObject& message ) >& callback );
    bool isCommunicationTestStarted();

private:
    Services::Service& m_hostService;
    StatisticsSettingsManager m_statisticSettingsManager;
};

}  // namespace Network
}  // namespace HubPlatform
