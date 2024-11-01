#include "opkg_utils/packages/local_command_runner.h"

namespace HubPlatform {
namespace Opkg {

OpenwrtCommandRunner::StdOutWithReturnCode LocalCommandRunner::listInstalled() const {
    std::string installedList = "firmware-addons-2gig - 1.0.0\n"
                                "firmware-addons-bluetooth - 1.0.0\n"
                                "firmware-addons-cameras - 1.0.0\n"
                                "firmware-addons-lua - 1.0.0\n"
                                "firmware-addons-zigbee - 1.0.0\n"
                                "firmware-addons-zwave - 1.0.0\n";
    return { installedList, true };
}

}  // namespace Opkg
}  // namespace HubPlatform
