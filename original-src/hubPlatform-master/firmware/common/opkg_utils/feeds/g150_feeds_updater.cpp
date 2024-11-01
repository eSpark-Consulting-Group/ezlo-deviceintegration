#include "opkg_utils/feeds/g150_feeds_updater.h"

#include <cstring>

namespace HubPlatform {
namespace Opkg {

namespace {
constexpr const char SysupgradeImagePath[] = "/g150_sysupgrade.bin";
constexpr const char RecoveryImagePath[] = "/g150_recovery.bin";
}  // namespace

std::string G150FeedsUpdater::getHubType() const {
    return "g150";
}

std::string G150FeedsUpdater::getUpdateVersion() const {
    return "v2";
}

std::string G150FeedsUpdater::getCurrentBaseUrl() const {
    if( getFeedsConfiguration().getFeedsCount() == 0 )
        return {};

    auto& url = getFeedsConfiguration().getFeed( 0 ).url;
    auto baseUrlEnd = url.find( "/system" );
    if( baseUrlEnd == std::string::npos )
        return {};

    return url.substr( 0, baseUrlEnd );
}

std::string G150FeedsUpdater::generateImageUrl() const {
    auto currentBaseUrl = getCurrentBaseUrl();
    if( currentBaseUrl.empty() )
        return {};

    return generateSysupgradeImageUrl( currentBaseUrl );
}

std::string G150FeedsUpdater::generateRecoveryImageUrl() const {
    auto currentBaseUrl = getCurrentBaseUrl();
    if( currentBaseUrl.empty() )
        return {};

    return generateRecoveryImageUrl( currentBaseUrl );
}

std::string G150FeedsUpdater::extractBaseUrlFromImageUrl( const std::string& imageUrl ) {
    std::string baseUrl;
    size_t suffixLength = strlen( SysupgradeImagePath );
    baseUrl = imageUrl.substr( 0, imageUrl.size() - suffixLength );
    return baseUrl;
}

std::string G150FeedsUpdater::generateSysupgradeImageUrl( const std::string& baseUrl ) {
    return baseUrl + SysupgradeImagePath;
}

std::string G150FeedsUpdater::generateRecoveryImageUrl( const std::string& baseUrl ) {
    return baseUrl + RecoveryImagePath;
}

std::string G150FeedsUpdater::extractBaseUrl( const std::string& imageUrl ) const {
    return extractBaseUrlFromImageUrl( imageUrl );
}

void G150FeedsUpdater::registerCommonFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "system", baseUrl + "/system" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "firmware", baseUrl + "/firmware" } );
    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "plugins", baseUrl + "/plugins" } );
}

void G150FeedsUpdater::registerCustomFeeds( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.addFeed( { Opkg::DownloadMethod::SrcGz, "custom_packages", baseUrl + "/custom_packages" } );
}

}  // namespace Opkg
}  // namespace HubPlatform
