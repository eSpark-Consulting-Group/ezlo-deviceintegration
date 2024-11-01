#include "opkg_utils/feeds/h2_feeds_updater.h"

#include <cstring>

namespace HubPlatform {
namespace Opkg {

namespace {
constexpr const char ImagePath[] = "/ezlo-openwrt-sunxi-cortexa7-sun8i-h2-plus-orangepi-zero-squashfs-sdcard.img.gz";
}  // namespace

std::string H2FeedsUpdater::getHubType() const {
    return "h2.1";
}

std::string H2FeedsUpdater::getUpdateVersion() const {
    return "v1";
}

std::string H2FeedsUpdater::getCurrentBaseUrl() const {
    if( getFeedsConfiguration().getFeedsCount() == 0 )
        return {};

    auto& url = getFeedsConfiguration().getFeed( 0 ).url;
    auto baseUrlEnd = url.find( "/system/" );
    if( baseUrlEnd == std::string::npos )
        return {};

    return url.substr( 0, baseUrlEnd );
}

std::string H2FeedsUpdater::generateImageUrl() const {
    auto currentBaseUrl = getCurrentBaseUrl();
    if( currentBaseUrl.empty() )
        return {};

    return currentBaseUrl + ImagePath;
}

std::string H2FeedsUpdater::generateRecoveryImageUrl() const {
    return generateImageUrl();
}

std::string H2FeedsUpdater::extractBaseUrl( const std::string& imageUrl ) const {
    std::string baseUrl;
    size_t suffixLength = strlen( ImagePath );
    baseUrl = imageUrl.substr( 0, imageUrl.size() - suffixLength );
    return baseUrl;
}

void H2FeedsUpdater::registerCommonFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_core", baseUrl + "/system/targets/sunxi/cortexa7/packages" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz,
                                  "ezlo_openwrt_base",
                                  baseUrl + "/system/packages/arm_cortex-a7_neon-vfpv4/base" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz,
                                  "ezlo_openwrt_hub_platform_deps",
                                  baseUrl + "/system/packages/arm_cortex-a7_neon-vfpv4/hub_platform_deps" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz,
                                  "ezlo_openwrt_packages",
                                  baseUrl + "/system/packages/arm_cortex-a7_neon-vfpv4/packages" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_hub_platform", baseUrl + "/firmware" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_hub_plugins", baseUrl + "/plugins" } );
}

/** These paths should correspond to ones in HubPlatformBundle repository, build_bundle.sh for a specific hub type, see
 * distfeeds.conf */
void H2FeedsUpdater::registerCustomFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_custom_packages", baseUrl + "/custom_packages" } );
}

}  // namespace Opkg
}  // namespace HubPlatform
