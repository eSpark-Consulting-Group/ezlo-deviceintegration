#include "opkg_utils/feeds/base_url_utils.h"

#include <cstring>

namespace HubPlatform {
namespace Opkg {

/*
Base URL format:
    <source+[environment]>/bundle/<hub type>/<update version>/[snapshots/<snapshot>|latest]

Base URL examples:
    http://dl.mios.com/linux_firmware/branch/EFWR-XXXX_smth_good/bundle/h2.1/v1/snapshots/1
    http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42
    http://dl.mios.com/linux_firmware/bundle/g150/v2/latest
*/

std::string BaseUrlUtils::composeBaseUrl( const std::string& source,
                                          const std::string& hubType,
                                          const std::string& updateVersion,
                                          int snapshot ) {
    std::string baseUrl;
    if( !source.empty() ) {
        baseUrl = source + "/bundle/" + hubType + "/" + updateVersion;
    }

    if( snapshot >= 0 ) {
        baseUrl += "/snapshots/" + std::to_string( snapshot );
    }
    else {
        baseUrl += "/latest";
    }
    return baseUrl;
}

std::string BaseUrlUtils::getSource( const std::string& baseUrl ) {
    auto sourceEnd = baseUrl.find( "/bundle/" );
    if( sourceEnd == std::string::npos )
        return {};

    return baseUrl.substr( 0, sourceEnd );
}

std::string BaseUrlUtils::getUpdateVersion( const std::string& baseUrl ) {
    auto updateVersionEnd = baseUrl.find( "/latest" );
    if( updateVersionEnd == std::string::npos ) {
        updateVersionEnd = baseUrl.find( "/snapshots/" );
        if( updateVersionEnd == std::string::npos ) {
            return {};
        }
    }
    auto updateVersionBegin = baseUrl.rfind( '/', updateVersionEnd - 1 ) + 1;

    return baseUrl.substr( updateVersionBegin, updateVersionEnd - updateVersionBegin );
}

int BaseUrlUtils::getSnapshot( const std::string& baseUrl ) {
    auto snapshotBegin = baseUrl.find( "/snapshots/" );
    if( snapshotBegin == std::string::npos )
        return -1;

    snapshotBegin += strlen( "/snapshots/" );

    try {
        return std::stoi( baseUrl.substr( snapshotBegin ) );
    }
    catch( ... ) {
        return -1;
    }
}

}  // namespace Opkg
}  // namespace HubPlatform