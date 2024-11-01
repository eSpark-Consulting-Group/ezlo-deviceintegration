#pragma once

#include "networkd/scan/nm_wifi_passive_scan_task.h"
#include "networkd/scan/nm_scan_result.h"
#include "networkd/scan/nm_scan_result_listener.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

namespace HubPlatform {
namespace Network {

class WifiPassiveScanTaskBase : public WifiPassiveScanTask {
public:
    WifiPassiveScanTaskBase( std::string interfaceName, WifiScanResultListener& scanResultListener );
    ~WifiPassiveScanTaskBase() override;

    void start() override;
    void stop() override;
    void stopWorkerAndWait() final;

    static Json manualScan( const std::string& interfaceName, const std::string& ssid = "" );

    std::chrono::milliseconds getScanInterval() const override;
    std::chrono::seconds getScanTimeout() const override;

protected:
    virtual void beforeScan();
    virtual void doScan();
    virtual void sendBroadcast();
    virtual void onStopped();

protected:
    enum ScanPriority { Background = 100, Manual = 200 };
    std::thread m_worker;
    std::atomic_bool m_stopped{ false };
    std::atomic_bool m_isRunning{ false };

    NetworkMap m_scanned;
    std::atomic< std::chrono::steady_clock::duration > m_scanStartedDuration;
};

}  // namespace Network
}  // namespace HubPlatform
