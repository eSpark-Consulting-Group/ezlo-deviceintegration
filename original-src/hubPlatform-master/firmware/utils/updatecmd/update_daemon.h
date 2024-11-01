#pragma once

#include "daemon_module/daemon_module.h"

#include "opkg_utils/opkg_manager.h"

namespace HubPlatform {
namespace UpdateCommand {

class UpdateDaemon final {
public:
    int run( int argc, char* argv[] );

private:
    bool factoryReset();
    bool softReset();
    bool restartFirmware();
    bool updateFirmware();
    bool switchToLuupFirmware();
    bool updateBootloader();
    bool updateRecoveryPartition();

    bool installDefaultPackages();

private:
    Opkg::OpkgManager m_opkg;
    bool m_isDaemon = false;
};
}  // namespace UpdateCommand
}  // namespace HubPlatform
