#pragma once

#include "service_interfaces/reporter_interface.h"
#include "ubus_interface/ubus_impl.h"
#include <string>

class Reporter : public ReporterInterface {
public:
    explicit Reporter( std::string name );
    ~Reporter() override;
    std::string prepareReportData() override;
    void sendReportData() override;
    int postInterval() const override;
    uint64_t getCPUTime() override;
    JsonObject getProcessMemory() override;
    std::string getProcStatus() override;
    int64_t getProcessUptimeTicks( const std::string& pid ) override;
    std::chrono::seconds getProcessUptimeSeconds( int64_t uptimeTicks ) override;

private:
    bool postRequest( const std::string& data );

private:
    const int DefaultPostInterval = 30;

    int m_postInterval = DefaultPostInterval;
    std::string m_serviceName;
    std::unique_ptr< HubPlatform::Ubus::UBusInterface > m_uBusObject = nullptr;
};
