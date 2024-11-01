#pragma once

#include "networkd/scan/nm_scan_result.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class WiFiScanException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
class WiFiScanResourceBusyException : public WiFiScanException {
public:
    WiFiScanResourceBusyException( const std::string& errDescription )
    : WiFiScanException( "WiFiScanResourceBusyException: " + errDescription ) {}
};

int convertWifiSignalDbmToPercent( double dbm );
NetworkMap scanWifiToMap( const std::string& interfaceName, const std::string& ssid = "" );
void setUpInterfaceForScan( const std::string& interfaceName );

std::string getBssid( const Json& wifiScanResult, const std::string& ssid );
NetworkItem getScannedNetwork( const Json& wifiScanResult, const std::string& ssid, const std::string& bssid = "" );
}  // namespace Network
}  // namespace HubPlatform
