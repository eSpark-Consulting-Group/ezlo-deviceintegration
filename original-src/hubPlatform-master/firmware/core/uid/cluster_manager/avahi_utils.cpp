#include "uid/cluster_manager/avahi_utils.h"

#include "json_utils/json_wrapper.h"

#include <sstream>

namespace HubPlatform {
namespace Uid {
namespace AvahiUtils {

// avahi-browse -lpktr _ezlo._tcp
// +;eth0;IPv4;eZLO\032h2\.1\032controller\032\04070010234\041;_ezlo._tcp;local
// =;eth0;IPv4;eZLO\032h2\.1\032controller\032\04070010234\041;_ezlo._tcp;local;HUB70010234.local;192.168.42.249;17000;
// "Hub Type=h2.1" "Vendor=eZLO" "Firmware Version=1.0.0" "Serial=70010234"

std::vector< Json > parseAvahiBrowseOutput( const std::string& output ) {
    std::vector< Json > result;
    std::stringstream outputStream( output );
    std::string line;
    while( getline( outputStream, line ) ) {
        std::stringstream lineStream( line );
        Json info;

        auto getNextField = [&]( std::string& field ) -> bool { return !getline( lineStream, field, ';' ).fail(); };

        auto getField = [&]( const std::string& fieldName ) -> bool {
            std::string value;
            if( !getNextField( value ) ) {
                return false;
            }
            info[ fieldName ] = value;
            return true;
        };

        std::string type;
        if( !getNextField( type ) || type != "=" ) {
            continue;
        }

        if( !getField( "interface" ) || !getField( "ipVersion" ) || !getField( "serviceName" ) ||
            !getField( "serviceType" ) || !getField( "domain" ) || !getField( "hostname" ) || !getField( "address" ) ||
            !getField( "port" ) ) {
            return result;
        }

        std::string records;
        if( !getNextField( records ) ) {
            return result;
        }

        // "Hub Type=h2.1" "Vendor=eZLO" "Firmware Version=1.0.0" "Serial=70010234"
        auto recordsJson = JSON_EMPTY_OBJ;
        size_t recordEndPos = 0;
        while( recordEndPos != std::string::npos ) {
            auto keyStartPos = records.find( '"', recordEndPos ) + 1;
            auto keyEndPos = records.find( '=', keyStartPos );
            if( keyEndPos == std::string::npos ) {
                return result;
            }
            auto valueStartPos = keyEndPos + 1;
            recordEndPos = records.find( ' ', valueStartPos );
            auto valueEndPos = recordEndPos == std::string::npos ? records.size() - 1 : recordEndPos - 1;
            if( records[ valueEndPos ] != '"' ) {
                return result;
            }
            std::string key = records.substr( keyStartPos, keyEndPos - keyStartPos );
            std::string value = records.substr( valueStartPos, valueEndPos - valueStartPos );
            recordsJson[ key ] = value;
        }
        info[ "records" ] = recordsJson;
        result.push_back( std::move( info ) );
    }

    return result;
}

}  // namespace AvahiUtils
}  // namespace Uid
}  // namespace HubPlatform
