#pragma once

#include <string>
#include <unordered_set>

#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Zigbee {

class AttributeStringNormalizer {
public:
    AttributeStringNormalizer() = delete;

    static void normalize( std::string& str ) {
        std::string originalString = str;
        // normalize string - remove control and special symbols
        for( size_t i = 0; i < str.size(); i++ ) {
            if( deleteSymbol( str[ i ] ) ) {
                str.erase( i--, 1 );
            }
        }

        if( str != originalString ) {
            LOGGER_DEBUG_RAW( "String attribute value from \"" + originalString + "\" normalized to \"" + str + "\"" );
        }
    }

private:
    static bool deleteSymbol( char s ) {
        static const std::unordered_set< char > allowedPunctuationCharacters{ '.', '-', '_', '+' };
        return !( ( std::isalnum( s ) || std::isspace( s ) ||
                    allowedPunctuationCharacters.find( s ) != allowedPunctuationCharacters.end() ) &&
                  !std::iscntrl( s ) );
    }
};

}  // namespace Zigbee
}  // namespace HubPlatform
