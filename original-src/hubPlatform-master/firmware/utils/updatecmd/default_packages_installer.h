#pragma once

#include "updatecmd/log.h"

#include "opkg_utils/packages/packages_manager.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include <memory>
#include <fstream>

namespace HubPlatform {
namespace UpdateCommand {

class DefaultPackagesInstaller final {
public:
    DefaultPackagesInstaller( std::unique_ptr< Opkg::IPackagesManager > packagesManager,
                              const std::string& logFileName );
    bool run();

private:
    std::unique_ptr< Opkg::IPackagesManager > m_packagesManager;
    std::ofstream m_log;
};

}  // namespace UpdateCommand
}  // namespace HubPlatform
