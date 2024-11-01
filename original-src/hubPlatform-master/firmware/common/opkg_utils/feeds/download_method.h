#pragma once

#include <stdint.h>
#include <string>

namespace HubPlatform {
namespace Opkg {

enum class DownloadMethod : uint8_t { Unknown, Src, SrcGz };

DownloadMethod downloadMethodFromString( const std::string& string );
const std::string& downloadMethodToString( DownloadMethod downloadMethod );

}  // namespace Opkg
}  // namespace HubPlatform