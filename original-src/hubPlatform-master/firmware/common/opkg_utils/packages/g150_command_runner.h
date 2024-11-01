#pragma once

#include "opkg_utils/packages/openwrt_command_runner.h"

namespace HubPlatform {
namespace Opkg {

class G150CommandRunner : public OpenwrtCommandRunner {
public:
    StdOutWithReturnCode sysupgrade( const std::string& imageUrl, const std::string& cacheDirectory = "" ) override;

private:
    static StdOutWithReturnCode updateRecovery( const std::string& imageUrl, const std::string& cacheDirectory );
    static StdOutWithReturnCode updateSystem( const std::string& imageUrl, const std::string& cacheDirectory );
};

}  // namespace Opkg
}  // namespace HubPlatform
