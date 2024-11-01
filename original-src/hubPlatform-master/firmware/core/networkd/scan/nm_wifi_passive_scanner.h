#pragma once
#include "networkd/scan/nm_wifi_passive_scan_task.h"
#include "networkd/scan/nm_scan_result_listener.h"
#include "networkd/scan/nm_scan_result.h"
#include "json_utils/json_fwd.h"

#include <map>
#include <string>
#include <memory>
#include <mutex>

namespace HubPlatform {

namespace Services {
class Service;
}

namespace Network {

class WifiPassiveScanner {
public:
    explicit WifiPassiveScanner( WifiScanResultListener& scanResultListener );

    void startScan( const std::string& interfaceName );
    void stopScan( const std::string& interfaceName );

    JsonArray manualScan( const std::string& interfaceName, const std::string& ssid = "" );
    NetworkItem scanAndGetNetwork( const std::string& interfaceName,
                                   const std::string& ssid,
                                   const std::string& bssid );

private:
    WifiScanResultListener& m_scanResultListener;

    std::mutex m_mutex;
    using ScanTaskPtr = std::unique_ptr< WifiPassiveScanTask >;
    std::map< std::string, ScanTaskPtr > m_scannerTasks;
};

}  // namespace Network
}  // namespace HubPlatform
