#pragma once

#include "networkd/scan/nm_wifi_passive_scan_task.h"

namespace HubPlatform {
namespace Network {

class DummyWifiPassiveScanTask : public WifiPassiveScanTask {
public:
    using WifiPassiveScanTask::WifiPassiveScanTask;

    void start() override{};
    void stop() override{};
    void stopWorkerAndWait() override{};

    static Json manualScan( const std::string& interfaceName, const std::string& ssid = "" ) {
        return {};
    };

    std::chrono::milliseconds getScanInterval() const override {
        return std::chrono::milliseconds( 1000 );
    };
    std::chrono::seconds getScanTimeout() const override {
        return std::chrono::seconds( 300 );
    };
};

}  // namespace Network
}  // namespace HubPlatform
