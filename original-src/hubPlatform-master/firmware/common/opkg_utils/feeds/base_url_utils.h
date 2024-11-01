#pragma once

#include <string>

namespace HubPlatform {
namespace Opkg {

class BaseUrlUtils {
public:
    static std::string composeBaseUrl( const std::string& source,
                                       const std::string& hubType,
                                       const std::string& updateVersion,
                                       int snapshot );

    static std::string getSource( const std::string& baseUrl );
    static std::string getUpdateVersion( const std::string& baseUrl );
    static int getSnapshot( const std::string& baseUrl );
};

}  // namespace Opkg
}  // namespace HubPlatform