#pragma once

#include "opkg_utils/feeds/base_feeds_updater.h"

#include "string"

namespace HubPlatform {
namespace Opkg {

class BeeWaveFeedsUpdater final : public BaseFeedsUpdater {
public:
    using BaseFeedsUpdater::BaseFeedsUpdater;
    std::string generateImageUrl() const override;
    std::string generateRecoveryImageUrl() const override;

private:
    std::string getHubType() const override;
    std::string getUpdateVersion() const override;
    std::string getCurrentBaseUrl() const override;
    std::string extractBaseUrl( const std::string& imageUrl ) const override;
    void registerCommonFeeds( const std::string& baseUrl ) override;
    void registerCustomFeeds( const std::string& baseUrl ) override;
};

}  // namespace Opkg
}  // namespace HubPlatform
