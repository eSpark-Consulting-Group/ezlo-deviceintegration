#pragma once

#include "opkg_utils/feeds/feeds_configuration.h"
#include "opkg_utils/feeds/ifeeds_updater.h"
#include "opkg_utils/keys/keys_manager.h"
#include "opkg_utils/packages/ipackages_manager.h"
#include "opkg_utils/packages/command_runner.h"

#include <memory>

namespace HubPlatform {
namespace Opkg {

class OpkgManager final {
public:
    std::unique_ptr< FeedsConfiguration > createFeedsConfiguration();
    std::unique_ptr< IFeedsUpdater > createFeedsUpdater(
        IFeedsUpdater::Feeds feedsType = IFeedsUpdater::Feeds::Common );
    std::unique_ptr< KeysManager > createKeysManager();
    std::unique_ptr< IPackagesManager > createPackagesManager();

private:
    CommandRunner& getCommandRunner();

private:
    std::unique_ptr< CommandRunner > m_commandRunner;
};

}  // namespace Opkg
}  // namespace HubPlatform
