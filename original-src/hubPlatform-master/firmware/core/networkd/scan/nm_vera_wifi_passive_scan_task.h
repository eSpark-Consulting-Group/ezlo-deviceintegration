#pragma once
#include "networkd/scan/nm_wifi_passive_scan_task_base.h"

namespace HubPlatform {
namespace Network {

class VeraWifiPassiveScanTask : public WifiPassiveScanTaskBase {
public:
    using WifiPassiveScanTaskBase::WifiPassiveScanTaskBase;
    ~VeraWifiPassiveScanTask() override;

    std::chrono::milliseconds getScanInterval() const override;
    std::chrono::seconds getBroadcastInterval() const;

protected:
    void doScan() override;
    void sendBroadcast() override;
    void onStopped() override;

protected:
    // Timepoint for sending broadcasts by specified in getBroadcastInterval() interval.
    std::chrono::steady_clock::time_point m_broadcastSentTp;
};

}  // namespace Network
}  // namespace HubPlatform