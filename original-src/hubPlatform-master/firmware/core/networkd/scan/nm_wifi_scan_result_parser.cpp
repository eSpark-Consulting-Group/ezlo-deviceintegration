#include "networkd/scan/nm_wifi_scan_result_parser.h"
#include "networkd/scan/nm_scan_utils.h"
#include "base_utils/utils.h"

#include <memory>
#include <algorithm>
#include <cassert>
#include <tiny/tiny_consolecolors.h>

namespace {
auto isSpace = []( unsigned char c ) -> bool { return std::isspace( c ); };

std::string unescapeIw( const std::string& value ) {
    std::string result;

    char const* s = value.c_str();
    unsigned char c;

    while( ( c = *s++ ) ) {
        if( c == '\\' && *s ) {
            switch( c = *s++ ) {
            case 'x': {
                unsigned char ch = *s;
                if( !isxdigit( ch ) )
                    goto not_an_escape;
                s++;
                c = hexToDec( ch );
                ch = *s;
                if( isxdigit( ch ) ) {
                    s++;
                    c = c * 16 + hexToDec( ch );
                }
            } break;
            not_an_escape:
            default:
                result += '\\';
                break;
            }
        }
        result += c;
    }
    return result;
}

std::string unescapeIwpriv( const std::string& value ) {
    std::string result;
    if( value.size() > 2 && value[ 0 ] == '0' && value[ 1 ] == 'x' ) {
        char const* s = value.c_str() + 2;
        while( *s ) {
            if( !isxdigit( *s ) ) {
                return value;
            }
            unsigned char c = hexToDec( *s );
            s++;
            if( !isxdigit( *s ) ) {
                return value;
            }
            c = c * 16 + hexToDec( *s );
            result += c;
            s++;
        }
        return result;
    }
    return value;
}

const std::string& getIwEncryption( const std::string& iwprivEncryption ) {
    static const std::string none = "none";
    static const std::string wep = "wep";
    static const std::string psk = "psk";
    static const std::string psk2 = "psk2";

    if( iwprivEncryption == "NONE" ) {
        return none;
    }
    else if( iwprivEncryption == "WEP" ) {
        return wep;
    }
    else if( iwprivEncryption.find( "2PSK" ) != std::string::npos ) {
        // E.g. WPA2PSK/AES, WPA1PSKWPA2PSK/AES, WPA1PSKWPA2PSK/TKIPAES
        return psk2;
    }
    else if( iwprivEncryption.find( "PSK" ) != std::string::npos ) {
        // E.g. WPAPSK/AES
        return psk;
    }
    // NOTE: return original encryption to make it easier to find unhandled encryptions
    return iwprivEncryption;
}

}  // namespace

namespace HubPlatform {
namespace Network {

bool WifiScanResultParser::parse( const std::string& scanResult ) {
    std::unique_ptr< WifiScanResultParser, void ( * )( WifiScanResultParser * parser ) > resetDataGuard(
        this, []( WifiScanResultParser* parser ) { parser->clearCurrentItem(); } );

    std::istringstream stream( scanResult );

    std::string line;
    while( std::getline( stream, line ) ) {
        parseLine( line );
    }

    tryAddScannedItem();

    return true;
}

const NetworkMap& WifiScanResultParser::getScannedItems() const {
    return m_scannedItems;
}

void WifiScanResultParser::setChannel( int channel ) {
    m_scannedValue.m_channel = std::move( channel );
}

void WifiScanResultParser::setSsid( const std::string& ssid ) {
    m_scannedValue.m_ssid = unescapeIw( ssid );
}

void WifiScanResultParser::setBssid( std::string bssid ) {
    m_scannedValue.m_bssid = std::move( bssid );
}

void WifiScanResultParser::setSignalPercentLevel( int level ) {
    assert( level >= 0 );
    assert( level <= 100 );

    m_scannedValue.m_signalPercentLevel = level;
}

const std::string& WifiScanResultParser::getSecurity() const {
    return m_scannedValue.m_security;
}

void WifiScanResultParser::setSecurity( std::string security ) {
    m_scannedValue.m_security = std::move( security );
}

void WifiScanResultParser::tryAddScannedItem() {
    if( m_scannedValue.isValid() ) {
        m_networksFound.push_back( m_scannedValue );

        auto it = m_scannedItems.find( m_scannedValue.m_ssid );
        if( it == m_scannedItems.end() || it->second.m_signalPercentLevel < m_scannedValue.m_signalPercentLevel ) {
            if( it == m_scannedItems.end() ) {
                m_scannedItems.emplace( m_scannedValue.m_ssid, std::move( m_scannedValue ) );
            }
            else {
                it->second = std::move( m_scannedValue );
            }
        }
    }

    clearCurrentItem();
}

void WifiScanResultParser::clearCurrentItem() {
    m_scannedValue.clear();
}

const WifiScanResultParser::NetworksList& WifiScanResultParser::getScannedItemsList() const {
    return m_networksFound;
}

void WifiScanResultParser::setAssociated( bool value ) {
    m_scannedValue.m_aux.m_associated = value;
}

void WifiScanResultParser::setSignalDBmLevel( double dbmLevel ) {
    m_scannedValue.m_aux.m_signal = dbmLevel;
}

void WifiScanResultParser::setFrequency( double frequency ) {
    m_scannedValue.m_aux.m_frequency = frequency;

    if( frequency > 5000 && frequency < 6000 ) {
        m_scannedValue.m_aux.m_band = NetworkItemAuxFields::Band5Ghz;
    }
    else if( frequency > 2000 && frequency < 3000 ) {
        m_scannedValue.m_aux.m_band = NetworkItemAuxFields::Band2_4Ghz;
    }
}

void WifiScanResultParser::setCapabilities( unsigned int capabilities ) {
    m_scannedValue.m_aux.m_capabilitiesFlags = capabilities;
}

void IwScanResultParser::parseLine( const std::string& line ) {
    auto lineIt = std::find_if_not( line.begin(), line.end(), isSpace );

    auto checkPrefix = [&lineIt, &line]( const char* prefix ) {
        auto it = lineIt;
        for( ; it != line.end() && *prefix && *prefix == *it; ++it, ++prefix )
            ;

        if( it != line.end() && isSpace( *it ) && !*prefix ) {
            lineIt = std::find_if_not( it + 1, line.end(), isSpace );
            return true;
        }

        return false;
    };

    if( checkPrefix( "BSS" ) ) {
        tryAddScannedItem();
        auto openBrace = std::find( lineIt, line.end(), '(' );
        setBssid( std::string( lineIt, openBrace ) );

        auto closeBrace = std::find( openBrace, line.end(), ')' );
        if( closeBrace != line.end() ) {
            std::string suffix( closeBrace + 1, line.end() );
            auto pos = suffix.find( "associated" );
            setAssociated( pos != std::string::npos );
        }
    }
    else if( checkPrefix( "SSID:" ) ) {
        setSsid( std::string( lineIt, line.end() ) );
    }
    else if( checkPrefix( "signal:" ) ) {
        try {
            double dbm = std::strtod( &*lineIt, nullptr );
            setSignalPercentLevel( convertWifiSignalDbmToPercent( dbm ) );
            setSignalDBmLevel( dbm );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::str( "WiFi Scan result: Error parsing signal dBm level: " ) + ex.what() );
        }
    }
    else if( checkPrefix( "capability:" ) ) {
        constexpr char capabilitiesMaskPrefix[] = "(0x";
        auto pos = line.find( capabilitiesMaskPrefix );
        if( pos != std::string::npos && pos + 5 < line.size() ) {
            auto it = line.begin() + pos + 5 /* "(0xXX" */;
            if( ( *it - '0' ) % 2 == 0 ) {
                // bit 5 (privacy) is not set
                // e.g. capability: ESS (0x0421)
                setSecurity( "none" );
            }
            else {
                // bit 5 (privacy) is set
                // e.g. capability: ESS (0x0431)
                if( getSecurity().empty() ) {
                    setSecurity( "wep" );
                }
            }
            std::string flagsString( line.begin() + pos + 1, line.end() );  // 0x1111)
            try {
                char* endPtr = nullptr;
                unsigned flags = std::strtod( flagsString.c_str(), &endPtr );
                setCapabilities( flags );
            }
            catch( const std::exception& ex ) {
                LOGGER_ERROR( cc::str( "WiFi Scan result: Error parsing network capabilities: " ) + ex.what() );
            }
        }
    }
    else if( checkPrefix( "HT" ) ) {
        if( getSecurity().empty() || getSecurity() == "wep" ) {
            setSecurity( "psk" );
        }
    }
    else if( checkPrefix( "RSN:" ) ) {
        setSecurity( "psk2" );
    }
    else if( checkPrefix( "* primary channel:" ) ) {
        setChannel( std::strtol( &*lineIt, nullptr, 10 ) );
    }
    else if( checkPrefix( "freq:" ) ) {
        try {
            double frequency = std::strtod( &*lineIt, nullptr );
            setFrequency( frequency );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::str( "WiFi Scan result: Error parsing network frequency: " ) + ex.what() );
        }
    }
    // TODO: should we extract channel by deducing it from frequency?
}

void IwprivScanResultParser::parseLine( const std::string& line ) {
    std::istringstream stream( line );
    int channel = 0;
    bool hiddenNetwork = false;

    /* clang-format off
	 *
	 *Ch  SSID                             BSSID               Security               Signal(%)W-Mode  ExtCH  NT WPS DPID
	 *7   kyky_ap                          4c:08:5g:b4:97:d1   WPA2PSK/AES            65       11b/g/n BELOW  In  NO
	 *2                                    4c:08:5g:b4:97:d2   WPA2PSK/AES            65       11b/g/n BELOW  In  NO
	 *
	 * clang-format on
	 */
    if( line[ 4 ] == ' ' ) {
        hiddenNetwork = true;
    }

    if( stream >> channel ) {
        std::string ssid, bssid, encryption;
        int signalLevel = 0;

        setChannel( channel );

        if( !hiddenNetwork ) {
            stream >> ssid;
        }

        if( stream >> bssid >> encryption >> signalLevel ) {
            setSsid( unescapeIwpriv( ssid ) );
            setBssid( std::move( bssid ) );
            setSecurity( getIwEncryption( encryption ) );
            setSignalPercentLevel( signalLevel );

            tryAddScannedItem();
        }
    }
}

}  // namespace Network
}  // namespace HubPlatform
