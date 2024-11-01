#include "opkg_utils/feeds/local_feeds_updater.h"

namespace HubPlatform {
namespace Opkg {

LocalFeedsUpdater::LocalFeedsUpdater( std::unique_ptr< Opkg::FeedsConfiguration > feedsConfiguration )
: m_feedsConfiguration( std::move( feedsConfiguration ) ) {}

bool LocalFeedsUpdater::update( const std::string& source, int snapshot ) {
    return true;
}
void LocalFeedsUpdater::update( const std::string& imageUrl ) {}
void LocalFeedsUpdater::resetCurrent(){};
void LocalFeedsUpdater::resetToCurrent(){};
std::string LocalFeedsUpdater::getFeedsSource() const {
    return "";
}
std::string LocalFeedsUpdater::getExpectedUpdateVersion() const {
    return "";
}
std::string LocalFeedsUpdater::getFeedsUpdateVersion() const {
    return "";
}
int LocalFeedsUpdater::getFeedsSnapshot() const {
    return 0;
}
std::string LocalFeedsUpdater::generateImageUrl() const {
    return "";
}
std::string LocalFeedsUpdater::generateRecoveryImageUrl() const {
    return "";
}
}  // namespace Opkg
}  // namespace HubPlatform
