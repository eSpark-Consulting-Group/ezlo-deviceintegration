#pragma once

#include <string>
#include <utility>
#include <functional>

namespace HubPlatform {
namespace OpenwrtUtils {

using StdOutWithReturnCode = std::pair< std::string, int >;
using Downloader = std::function< StdOutWithReturnCode( const std::string& url, const std::string& file ) >;

StdOutWithReturnCode downloadImage( const Downloader& downloader,
                                    const std::string& imageUrl,
                                    const std::string& directory );
StdOutWithReturnCode replaceMD5ChecksumPath( const std::string& checksumFilePath, const std::string& targetFilePath );
StdOutWithReturnCode verifyMD5Checksum( const std::string& checksumFilePath );

}  // namespace OpenwrtUtils
}  // namespace HubPlatform
