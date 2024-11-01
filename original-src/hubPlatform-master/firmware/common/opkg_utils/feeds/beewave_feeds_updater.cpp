#include "opkg_utils/feeds/beewave_feeds_updater.h"

#include <cstring>

namespace HubPlatform {
namespace Opkg {

namespace {
constexpr const char ImagePath[] = "/ezlo-openwrt-ramips-mt76x8-ezlo_mediatek-7688-squashfs-sysupgrade.bin";
constexpr const char RecoveryImagePath[] = "/ezlo-openwrt-ramips-mt76x8-ezlo_mediatek-7688-squashfs-recovery.bin";
}  // namespace

std::string BeeWaveFeedsUpdater::getHubType() const {
    return "beewave.1";
}

std::string BeeWaveFeedsUpdater::getUpdateVersion() const {
    return "v1";
}

std::string BeeWaveFeedsUpdater::getCurrentBaseUrl() const {
    if( getFeedsConfiguration().getFeedsCount() == 0 )
        return {};

    auto& url = getFeedsConfiguration().getFeed( 0 ).url;
    auto baseUrlEnd = url.find( "/system/" );
    if( baseUrlEnd == std::string::npos )
        return {};

    return url.substr( 0, baseUrlEnd );
}

std::string BeeWaveFeedsUpdater::generateImageUrl() const {
    auto currentBaseUrl = getCurrentBaseUrl();
    if( currentBaseUrl.empty() )
        return {};

    return currentBaseUrl + ImagePath;
}

std::string BeeWaveFeedsUpdater::generateRecoveryImageUrl() const {
    auto currentBaseUrl = getCurrentBaseUrl();
    if( currentBaseUrl.empty() )
        return {};

    return currentBaseUrl + RecoveryImagePath;
}

std::string BeeWaveFeedsUpdater::extractBaseUrl( const std::string& imageUrl ) const {
    std::string baseUrl;
    size_t suffixLength = strlen( ImagePath );
    baseUrl = imageUrl.substr( 0, imageUrl.size() - suffixLength );
    return baseUrl;
}

void BeeWaveFeedsUpdater::registerCommonFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_core", baseUrl + "/system/targets/ramips/mt76x8/packages" } );
    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_base", baseUrl + "/system/packages/mipsel_24kc/base" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz,
                                  "ezlo_openwrt_hub_platform_deps",
                                  baseUrl + "/system/packages/mipsel_24kc/hub_platform_deps" } );
    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_packages", baseUrl + "/system/packages/mipsel_24kc/packages" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_hub_platform", baseUrl + "/firmware" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_hub_plugins", baseUrl + "/plugins" } );
}

void BeeWaveFeedsUpdater::registerCustomFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed(
        { Opkg::DownloadMethod::SrcGz, "ezlo_openwrt_custom_packages", baseUrl + "/custom_packages" } );
}

}  // namespace Opkg
}  // namespace HubPlatform
