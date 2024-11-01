#pragma once

#include "data_receiver.h"
#include "data_sender.h"
#include "service_interfaces/service.h"
#include "config_reader/config_reader.h"

#include <string>

namespace HubPlatform {

namespace Metrics {

class Service : public Services::Service {
public:
    Service();

    DataReceiver& getReceiver();
    DataSender* getSender();

protected:
    void setUp() override;
    void tearDown() override;

    JsonObject getServiceInfo() override;

private:
    void setConfigIntOption( ConfigReader& configReader, const char* optionPath, int& setValue, int defaultValue );
    void prepareCustomServicesReporter();
    static bool isServiceRunning( const std::string& serviceName );

private:
    InternalMetricsPtr m_internalMetrics;
    DataReceiver m_receiver;

    std::thread m_getMetricsLoopThread;
    std::shared_ptr< std::promise< void > > m_promiseDataLoop;

    std::thread m_watchdogThread;
    std::condition_variable m_watchdogCondition;
    std::mutex m_intervalMutex;

    std::string m_cloudUrl;
    std::vector< size_t > m_cloudPorts;
    int m_sendIntervalNetworkDefault;
    int m_sendIntervalNetworkAnomaly;
    int m_getRequestInterval;

    std::string m_serverCrtFile;
    std::string m_clientCrtFile;
    std::string m_clientKeyFile;

    const std::chrono::seconds m_quitLoopDurationSec{ 1 };

    std::map< std::string, std::string > m_serviceFeatureMap;
};
}  // namespace Metrics
}  // namespace HubPlatform
