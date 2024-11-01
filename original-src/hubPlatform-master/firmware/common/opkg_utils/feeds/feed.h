#pragma once

#include "opkg_utils/feeds/download_method.h"

#include <string>

namespace HubPlatform {
namespace Opkg {

struct Feed final {
    static Feed fromString( const std::string& string );
    static std::string toString( const Feed& feed );

    bool isValid() const;

    DownloadMethod downloadMethod;
    std::string name;
    std::string url;
};

}  // namespace Opkg
}  // namespace HubPlatform