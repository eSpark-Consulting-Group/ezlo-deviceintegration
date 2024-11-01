#include "networkd/scan/nm_scan_utils.h"
#include "networkd/scan/nm_wifi_scan_result_parser.h"
#include "networkd/resources/nm_resources.h"
#include "networkd/utils/nm_utils.h"

#include "system_utils/system.h"
#include "json_utils/json_utils.h"
#include "base_utils/utils.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <net/if.h>

namespace HubPlatform {
namespace Network {

int convertWifiSignalDbmToPercent( double dbm ) {
    if( dbm < -92. ) {
        return 1;
    }

    if( dbm > -21. ) {
        return 100;
    }

    return round( ( -0.0154 * dbm - 0.3794 ) * dbm + 98.182 );
}

NetworkMap scanWifiToMap( const std::string& interfaceName, const std::string& ssid ) {
    LOGGER_TRACE( cc::trace( "interface: " ) + interfaceName + cc::trace( ", ssid: " ) + ssid );

    auto isSpace = []( unsigned char c ) -> bool { return std::isspace( c ); };

    if( std::all_of( interfaceName.begin(), interfaceName.end(), isSpace ) ) {
        return {};
    }

    std::string clarifyingSsid;

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    if( !ssid.empty() ) {
        clarifyingSsid = " " + ssid;
    }
    const std::string command = "iwpriv " + interfaceName + " get_site_survey" + clarifyingSsid;

    IwprivScanResultParser parser;
#else
    if( !ssid.empty() ) {
        clarifyingSsid = " ssid " + escapeSymbolsByFirst( ssid, "\\'\" #" );
    }

    setUpInterfaceForScan( interfaceName );

    const std::string command = "iw dev " + interfaceName + " scan" + clarifyingSsid;

    IwScanResultParser parser;
#endif

    System::CommandResult result;
    result = System::getCommandOutputWithErrorReport( command.c_str(), true );
    System::errorReport( result );
    auto& output = result.output;
    if( result.exitCode != 0 ) {
        LOGGER_ERROR( cc::error( "Scan failed, code: " ) + cc::num10( result.exitCode ) + cc::error( ", error: " ) +
                      strerror( errno ) );

        if( std::count( output.begin(), output.end(), '\n' ) > 1 ) {
            // NOTE: sometimes scan fails with (almost) complete normal output,
            //       which is huge and does not contain any errors
            if( output.find( "command failed: " ) != std::string::npos ) {
                if( output.find( "Resource busy (-16)" ) != std::string::npos ) {
                    throw WiFiScanResourceBusyException( output );
                }
                else {
                    throw WiFiScanException( output );
                }
            }

            output = "";
        }
        throw WiFiScanException( output );
    }
    parser.parse( output );
    LOGGER_DEBUG( cc::debug( "Number of wireless networks found: " ) + cc::num10( parser.getScannedItems().size() ) );
#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_G450__ ) && !defined( __HUB_TYPE_G550__ )
    //    Only for h2 hubs
    auto& networksRawList = parser.getScannedItemsList();
    JsonArray rawNetworks = JSON_EMPTY_ARR;
    const bool useAuxFields = true;
    for( auto it = networksRawList.begin(); it != networksRawList.end(); it++ ) {
        rawNetworks.push_back( it->toJson( useAuxFields ) );
    }

    if( !networksRawList.empty() ) {
        // Print raw networks fields
        LOGGER_DEBUG( "Raw networks found by wifi scan: " + rawNetworks.dump() );
    }
    // TODO: For E hubs, get another output of this method
#endif
    return parser.getScannedItems();
}

void setUpInterfaceForScan( const std::string& interfaceName ) {
    auto interfaces = Utils::getSystemInterfaces( { interfaceName } );
    if( interfaces.empty() ) {
        throw WiFiScanException( "Interface for scan does not exists: " + interfaceName );
    }

    if( interfaces.front().m_ifaFlags & IFF_UP ) {
        return;
    }

    LOGGER_WARN( cc::warn( "Interface for wifi scanning is not up, setting up: " ) + cc::note( interfaceName ) );

    {
        std::string enableInterfaceCmd = "ip link set " + interfaceName + " up";
        auto result = System::getCommandOutputWithErrorReport( enableInterfaceCmd.c_str(), true );
        System::errorReport( result );
        auto& output = result.output;
        if( result.exitCode != 0 ) {
            throw WiFiScanException( "Unable set-up scanning interface: " + interfaceName );
        }
    }

    if( !waitForCondition(
            [&interfaceName] {
                auto interface = Utils::getSystemInterfaces( { interfaceName } );
                return !interface.empty() && interface.front().m_ifaFlags & IFF_UP;
            },
            std::chrono::milliseconds{ 1000 },
            std::chrono::milliseconds{ 100 } ) ) {
        throw WiFiScanException( "Timeout for setting up scanning interface: " + cc::note( interfaceName ) );
    }
}

/** TODO: EFWR-4353: Modify or create another method to get network item with new fields */
NetworkItem getScannedNetwork( const Json& wifiScanResult, const std::string& ssid, const std::string& bssid ) {
    if( !wifiScanResult.is_object() ) {
        return {};
    }

    const auto& networksList = wifiScanResult[ std::string( "networks" ) ];

    if( !networksList.is_array() || ssid.empty() ) {
        return {};
    }

    int strongestSignal = std::numeric_limits< int >::min();
    NetworkItem result;

    int networkIndex = -1;

    for( size_t i = 0; i < networksList.size(); ++i ) {
        const Json& network = networksList.at( i );

        if( json_get_str( network, "ssid" ) != ssid ) {
            continue;
        }

        auto networkBSSID = json_get_str( network, "bssid" );
        if( networkBSSID.empty() ) {
            continue;
        }

        if( !bssid.empty() && networkBSSID == bssid ) {
            networkIndex = i;
            break;
        }

        const auto currentLevel = json_get_int( network, "signalLevel" );
        if( currentLevel <= strongestSignal ) {
            continue;
        }

        strongestSignal = currentLevel;
        networkIndex = i;
    }

    if( networkIndex < 0 )
        return {};

    NetworkItem item;
    item.fromJson( networksList.at( networkIndex ), false );
    return item;
}

std::string getBssid( const Json& wifiScanResult, const std::string& ssid ) {
    if( !wifiScanResult.is_object() ) {
        return {};
    }

    const auto& list = wifiScanResult[ std::string( "networks" ) ];

    if( !list.is_array() || ssid.empty() ) {
        return {};
    }

    int level = std::numeric_limits< int >::min();
    std::string result;

    for( const Json& item : list ) {
        if( json_get_str( item, "ssid" ) != ssid ) {
            continue;
        }

        auto bssid = json_get_str( item, "bssid" );
        if( bssid.empty() ) {
            continue;
        }

        const auto currentLevel = json_get_int( item, "signalLevel" );
        if( currentLevel <= level ) {
            continue;
        }

        result = std::move( bssid );
        level = currentLevel;
    }

    return result;
}

}  // namespace Network
}  // namespace HubPlatform
