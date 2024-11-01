#include "opkg_utils/feeds/feed.h"

#include <sstream>
#include <fstream>

namespace HubPlatform {
namespace Opkg {

Feed Feed::fromString( const std::string& string ) {
    std::istringstream iss( string );
    std::string methodString, name, url;
    if( !( iss >> methodString >> name >> url ) )
        return {};

    if( !iss.eof() )
        return {};

    auto method = downloadMethodFromString( methodString );
    if( method == DownloadMethod::Unknown )
        return {};

    return { std::move( method ), std::move( name ), std::move( url ) };
}

std::string Feed::toString( const Feed& feed ) {
    if( !feed.isValid() )
        return {};

    return downloadMethodToString( feed.downloadMethod ) + " " + feed.name + " " + feed.url;
}

bool Feed::isValid() const {
    return downloadMethod != DownloadMethod::Unknown;
}

}  // namespace Opkg
}  // namespace HubPlatform
