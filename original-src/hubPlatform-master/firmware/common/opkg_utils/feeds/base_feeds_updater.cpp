#include <tiny/tiny_consolecolors.h>
#include "opkg_utils/feeds/base_feeds_updater.h"
#include "opkg_utils/feeds/base_url_utils.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Opkg {

BaseFeedsUpdater::BaseFeedsUpdater( std::unique_ptr< Opkg::FeedsConfiguration > feedsConfiguration, Feeds feedType )
: m_feedsConfiguration( std::move( feedsConfiguration ) )
, m_feedsType( feedType ) {}

bool BaseFeedsUpdater::update( const std::string& source, int snapshot ) {
    LOGGER_INFO( cc::info( "BaseFeedUpdater::update( source, snapshot ) : " ) + source + ", " + cc::num10( snapshot ) );

    std::string currentBaseUrl;
    if( source.empty() || snapshot == 0 ) {
        currentBaseUrl = getCurrentBaseUrl();
        if( currentBaseUrl.empty() )
            return false;
    }

    auto actualSource = source.empty() ? BaseUrlUtils::getSource( currentBaseUrl ) : source;
    auto actualSnapshot = snapshot == 0 ? BaseUrlUtils::getSnapshot( currentBaseUrl ) : snapshot;
    auto baseUrl = BaseUrlUtils::composeBaseUrl( actualSource, getHubType(), getUpdateVersion(), actualSnapshot );

    updateFromBaseUrl( baseUrl );
    return true;
}

void BaseFeedsUpdater::update( const std::string& imageUrl ) {
    auto baseUrl = extractBaseUrl( imageUrl );
    updateFromBaseUrl( baseUrl );
}

void BaseFeedsUpdater::resetCurrent() {
    m_feedsConfiguration->removeBackup();
}

void BaseFeedsUpdater::resetToCurrent() {
    m_feedsConfiguration->tryRestore();
}

std::string BaseFeedsUpdater::getFeedsSource() const {
    return BaseUrlUtils::getSource( getCurrentBaseUrl() );
}

std::string BaseFeedsUpdater::getExpectedUpdateVersion() const {
    return getUpdateVersion();
}

std::string BaseFeedsUpdater::getFeedsUpdateVersion() const {
    return BaseUrlUtils::getUpdateVersion( getCurrentBaseUrl() );
}

int BaseFeedsUpdater::getFeedsSnapshot() const {
    return BaseUrlUtils::getSnapshot( getCurrentBaseUrl() );
}

Opkg::FeedsConfiguration& BaseFeedsUpdater::getFeedsConfiguration() {
    return *m_feedsConfiguration;
}

const Opkg::FeedsConfiguration& BaseFeedsUpdater::getFeedsConfiguration() const {
    return *m_feedsConfiguration;
}

void BaseFeedsUpdater::updateFromBaseUrl( const std::string& baseUrl ) {
    auto& feedsConfiguration = getFeedsConfiguration();

    feedsConfiguration.clearFeeds();

    registerFeeds( baseUrl );

    feedsConfiguration.tryBackup();
    feedsConfiguration.commit();
}

void BaseFeedsUpdater::registerFeeds( const std::string& baseUrl ) {
    // DONE: EFWR-4028: Separate feeds: Common, Custom, All
    bool needFeedsAll = BaseFeedsUpdater::getFeedsType() == Feeds::All;
    bool needFeedsCommon = needFeedsAll || BaseFeedsUpdater::getFeedsType() == Feeds::Common;
    bool needFeedsCustom = needFeedsAll || BaseFeedsUpdater::getFeedsType() == Feeds::Custom;

    if( needFeedsCommon ) {
        LOGGER_DEBUG( cc::debug( "Need common feeds" ) );
        registerCommonFeeds( baseUrl );
    }

    if( needFeedsCustom ) {
        LOGGER_DEBUG( cc::debug( "Need custom feeds" ) );
        registerCustomFeeds( baseUrl );
    }
}

void BaseFeedsUpdater::registerCustomFeeds( const std::string& baseUrl ) {
    Log::warn( cc::warn( "Custom feeds aren't defined for the controller type!" ) );
}

}  // namespace Opkg
}  // namespace HubPlatform
