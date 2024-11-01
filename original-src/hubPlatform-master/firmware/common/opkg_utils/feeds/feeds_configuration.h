#pragma once

#include "opkg_utils/feeds/feed.h"

#include <vector>
#include <memory>

namespace HubPlatform {
namespace Opkg {

class FeedsConfiguration final {
public:
    FeedsConfiguration( std::string feedsConfigurationPath );

    int getFeedsCount() const;
    const Feed& getFeed( int idx ) const;

    void addFeed( Feed feed );
    Feed& takeFeed( int idx );
    void removeFeed( int idx );
    void clearFeeds();

    void revert();
    void commit();

    bool removeBackup();
    bool tryBackup();
    bool tryRestore();

private:
    using Feeds = std::vector< Feed >;

    Feeds readFeeds() const;
    void writeFeeds( Feeds feeds );

private:
    std::string m_feedsConfigurationPath;
    std::string m_feedsConfigurationBackupPath;
    Feeds m_feedsSnapshot;
};

}  // namespace Opkg
}  // namespace HubPlatform