#include "networkd/scan/nm_vera_wifi_passive_scan_task.h"

namespace HubPlatform {
namespace Network {

VeraWifiPassiveScanTask::~VeraWifiPassiveScanTask() {
    stopWorkerAndWait();
}

std::chrono::milliseconds VeraWifiPassiveScanTask::getScanInterval() const {
    return std::chrono::seconds( 1 );
}

std::chrono::seconds VeraWifiPassiveScanTask::getBroadcastInterval() const {
    return std::chrono::seconds( 8 );
}

void VeraWifiPassiveScanTask::doScan() {
    // Without this command new networks do not appear in scan result.
    system( "iwpriv apcli0 set SiteSurvey=0" );

    WifiPassiveScanTaskBase::doScan();
}

void VeraWifiPassiveScanTask::sendBroadcast() {
    // When this method is called first time, force broadcast sending.
    if( m_broadcastSentTp != std::chrono::steady_clock::time_point() ) {
        auto duration = std::chrono::steady_clock::now() - m_broadcastSentTp;
        if( duration < getBroadcastInterval() ) {
            return;
        }
    }

    WifiPassiveScanTaskBase::sendBroadcast();
    m_broadcastSentTp = std::chrono::steady_clock::now();
}

void VeraWifiPassiveScanTask::onStopped() {
    WifiPassiveScanTaskBase::onStopped();
    m_broadcastSentTp = std::chrono::steady_clock::time_point();
}

}  // namespace Network
}  // namespace HubPlatform