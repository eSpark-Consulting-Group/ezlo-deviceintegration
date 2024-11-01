#include "opkg_utils/feeds/download_method.h"

#include <unordered_map>

namespace HubPlatform {
namespace Opkg {

namespace {

struct DownloadMethodHash {
    size_t operator()( const DownloadMethod& method ) const {
        return static_cast< size_t >( method );
    }
};

const std::unordered_map< DownloadMethod, std::string, DownloadMethodHash > downloadMetdhodToStringMap = {
    { DownloadMethod::Unknown, "" },
    { DownloadMethod::Src, "src" },
    { DownloadMethod::SrcGz, "src/gz" },
};

const std::unordered_map< std::string, DownloadMethod > stringToDownloadMetdhodMap = [&]() {
    std::unordered_map< std::string, DownloadMethod > map;
    for( auto pair : downloadMetdhodToStringMap ) {
        map.emplace( std::make_pair( pair.second, pair.first ) );
    }
    return map;
}();

}  // namespace

DownloadMethod downloadMethodFromString( const std::string& string ) {
    auto it = stringToDownloadMetdhodMap.find( string );
    if( it != stringToDownloadMetdhodMap.end() ) {
        return it->second;
    }
    return DownloadMethod::Unknown;
}

const std::string& downloadMethodToString( DownloadMethod downloadMethod ) {
    auto it = downloadMetdhodToStringMap.find( downloadMethod );
    if( it != downloadMetdhodToStringMap.end() ) {
        return it->second;
    }
    return downloadMetdhodToStringMap.find( DownloadMethod::Unknown )->second;
}

}  // namespace Opkg
}  // namespace HubPlatform