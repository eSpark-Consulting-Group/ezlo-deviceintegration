#include "opkg_utils/opkg_manager.h"

#include "opkg_utils/keys/keys_manager.h"
#include "opkg_utils/feeds/h2_feeds_updater.h"
#include "opkg_utils/feeds/h2_secure_feeds_updater.h"
#include "opkg_utils/feeds/g150_feeds_updater.h"
#include "opkg_utils/feeds/beewave_feeds_updater.h"
#include "opkg_utils/feeds/local_feeds_updater.h"
#include "opkg_utils/packages/packages_manager.h"

#include "opkg_utils/packages/openwrt_command_runner.h"
#include "opkg_utils/packages/g150_command_runner.h"
#include "opkg_utils/packages/local_command_runner.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Opkg {

namespace {

constexpr const char DistFeedsConfPath[] = "/etc/opkg/distfeeds.conf";
constexpr const char KeysDirectory[] = "/etc/opkg/keys";

}  // namespace

std::unique_ptr< FeedsConfiguration > OpkgManager::createFeedsConfiguration() {
    return std::make_unique< FeedsConfiguration >( DistFeedsConfPath );
}

std::unique_ptr< IFeedsUpdater > OpkgManager::createFeedsUpdater( IFeedsUpdater::Feeds feedsType ) {
#ifdef __HUB_TYPE_H2__
    return std::make_unique< H2FeedsUpdater >( createFeedsConfiguration(), feedsType );
#elif __HUB_TYPE_H2_SECURE__
    return std::make_unique< H2SecureFeedsUpdater >( createFeedsConfiguration(), feedsType );
#elif __HUB_TYPE_G150__
    return std::make_unique< G150FeedsUpdater >( createFeedsConfiguration(), feedsType );
#elif __HUB_TYPE_BEEWAVE__
    return std::make_unique< BeeWaveFeedsUpdater >( createFeedsConfiguration(), feedsType );
#elif __HUB_TYPE_LOCAL__
    return std::make_unique< LocalFeedsUpdater >( createFeedsConfiguration() );
#else
    return nullptr;
#endif
}

std::unique_ptr< KeysManager > OpkgManager::createKeysManager() {
    return std::make_unique< KeysManager >( KeysDirectory );
}

std::unique_ptr< IPackagesManager > OpkgManager::createPackagesManager() {
    return std::make_unique< PackagesManager >( getCommandRunner() );
}

CommandRunner& OpkgManager::getCommandRunner() {
    if( !m_commandRunner ) {
#if defined( __HUB_TYPE_G150__ )
        m_commandRunner = std::make_unique< G150CommandRunner >();
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
        m_commandRunner = std::make_unique< LocalCommandRunner >();
#else
        m_commandRunner = std::make_unique< OpenwrtCommandRunner >();
#endif
    }
    return *m_commandRunner;
}

}  // namespace Opkg
}  // namespace HubPlatform
