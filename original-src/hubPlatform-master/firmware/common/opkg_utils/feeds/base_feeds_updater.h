#pragma once

#include "opkg_utils/feeds/ifeeds_updater.h"
#include "opkg_utils/feeds/feeds_configuration.h"

namespace HubPlatform {
namespace Opkg {

class BaseFeedsUpdater : public IFeedsUpdater {
public:
    BaseFeedsUpdater( std::unique_ptr< Opkg::FeedsConfiguration > feedsConfiguration, Feeds feedType = Feeds::All );

    bool update( const std::string& source, int snapshot ) override;
    void update( const std::string& imageUrl ) override;

    void resetCurrent() override;
    void resetToCurrent() override;

    std::string getFeedsSource() const override;
    std::string getExpectedUpdateVersion() const override;
    std::string getFeedsUpdateVersion() const override;
    int getFeedsSnapshot() const override;
    Feeds getFeedsType() const override {
        return m_feedsType;
    }

protected:
    virtual std::string getHubType() const = 0;
    virtual std::string getUpdateVersion() const = 0;
    virtual std::string getCurrentBaseUrl() const = 0;
    virtual std::string extractBaseUrl( const std::string& imageUrl ) const = 0;
    virtual void updateFromBaseUrl( const std::string& baseUrl );
    virtual void registerFeeds( const std::string& baseUrl );
    virtual void registerCommonFeeds( const std::string& baseUrl ) = 0;
    virtual void registerCustomFeeds( const std::string& baseUrl );

    Opkg::FeedsConfiguration& getFeedsConfiguration();
    const Opkg::FeedsConfiguration& getFeedsConfiguration() const;

private:
    std::unique_ptr< Opkg::FeedsConfiguration > m_feedsConfiguration;
    Feeds m_feedsType{ Feeds::All };
};

}  // namespace Opkg
}  // namespace HubPlatform
