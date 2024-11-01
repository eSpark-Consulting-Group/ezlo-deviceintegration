#pragma once

#include "opkg_utils/feeds/ifeeds_updater.h"
#include "opkg_utils/feeds/feeds_configuration.h"

namespace HubPlatform {
namespace Opkg {

class LocalFeedsUpdater final : public IFeedsUpdater {
public:
    LocalFeedsUpdater( std::unique_ptr< Opkg::FeedsConfiguration > feedsConfiguration );

    bool update( const std::string& source, int snapshot ) override;
    void update( const std::string& imageUrl ) override;

    void resetCurrent() override;
    void resetToCurrent() override;

    std::string getFeedsSource() const override;
    std::string getExpectedUpdateVersion() const override;
    std::string getFeedsUpdateVersion() const override;
    int getFeedsSnapshot() const override;

    std::string generateImageUrl() const override;
    std::string generateRecoveryImageUrl() const override;

private:
    std::unique_ptr< Opkg::FeedsConfiguration > m_feedsConfiguration;
};

}  // namespace Opkg
}  // namespace HubPlatform
