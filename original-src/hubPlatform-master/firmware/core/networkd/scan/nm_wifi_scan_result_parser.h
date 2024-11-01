#pragma once

#include "networkd/scan/nm_scan_result.h"
#include <vector>

namespace HubPlatform {
namespace Network {

class WifiScanResultParser {
public:
    WifiScanResultParser() = default;
    virtual ~WifiScanResultParser() = default;

    using ScannedValue = NetworkItem;
    using NetworksList = std::vector< ScannedValue >;

    bool parse( const std::string& scanResult );
    const NetworkMap& getScannedItems() const;
    const NetworksList& getScannedItemsList() const;
    const std::string& getSecurity() const;

protected:
    virtual void parseLine( const std::string& line ) = 0;

    void setChannel( int channel );
    void setSsid( const std::string& ssid );
    void setBssid( std::string bssid );
    void setSignalPercentLevel( int level );
    void setSignalDBmLevel( double dbmLevel );
    void setFrequency( double frequency );
    void setCapabilities( unsigned capabilities );

    void setSecurity( std::string security );
    void setAssociated( bool value );
    void tryAddScannedItem();

private:
    void clearCurrentItem();

private:
    ScannedValue m_scannedValue;
    NetworksList m_networksFound;

    NetworkMap m_scannedItems;
};

class IwScanResultParser : public WifiScanResultParser {
protected:
    void parseLine( const std::string& line ) override;
};

class IwprivScanResultParser : public WifiScanResultParser {
protected:
    void parseLine( const std::string& line ) override;
};

}  // namespace Network
}  // namespace HubPlatform
