#pragma once

#include "config_reader/config_reader.h"

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <exception>

namespace HubPlatform {
namespace Network {

class NetworkConfigFileManager final {
public:
    class Error : public std::logic_error {
    public:
        using logic_error::logic_error;
    };

    struct MobileConfiguration final {
        std::vector< std::string > m_whiteOperatorsList;
        bool m_autoRegistration = true;
        std::chrono::seconds m_pingPeriodNotActive;
        std::chrono::seconds m_pingPeriodActive;
        std::chrono::seconds m_modemWatchdogTimer;
        std::chrono::seconds m_finallyFailedToRegisterTimeout;
        std::chrono::seconds m_connectionRevivalAttemptTimeout;
        std::chrono::seconds m_modemMonitoringThreadPollingPeriod;
        std::chrono::seconds m_modemInfoPollingPeriod;

        using APNSettings = std::map< std::string, std::vector< std::string > >;
        APNSettings m_APNSettings;
        std::vector< std::string > m_defaultAPNSettings;
    };

    NetworkConfigFileManager();
    NetworkConfigFileManager( ConfigReader& readerInstance );

    const MobileConfiguration& getMobileConfiguration() const;

private:
    void init();
    void constructMobileConfiguration();

private:
    ConfigReader m_defaultReader;
    std::reference_wrapper< ConfigReader > m_configReader;
    MobileConfiguration m_mobileConfiguration;
};

}  // namespace Network
}  // namespace HubPlatform
