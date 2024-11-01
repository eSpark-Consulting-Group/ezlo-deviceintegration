#pragma once

#include "networkd/scan/nm_scan_result_listener.h"

#include <chrono>

namespace HubPlatform {
namespace Network {

class WifiPassiveScanTask {
public:
    WifiPassiveScanTask( std::string interfaceName, WifiScanResultListener& scanResultListener )
    : m_interfaceName( std::move( interfaceName ) )
    , m_scanResultListener( scanResultListener ){};

    virtual ~WifiPassiveScanTask() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void stopWorkerAndWait() = 0;

    virtual std::chrono::milliseconds getScanInterval() const = 0;
    virtual std::chrono::seconds getScanTimeout() const = 0;

protected:
    const std::string m_interfaceName;
    WifiScanResultListener& m_scanResultListener;
};

}  // namespace Network
}  // namespace HubPlatform
