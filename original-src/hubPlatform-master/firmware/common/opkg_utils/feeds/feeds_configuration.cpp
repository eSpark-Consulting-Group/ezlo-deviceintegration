#include "opkg_utils/feeds/feeds_configuration.h"

#include "common_utils/cpp14.h"
#include "system_utils/filesystem.h"

#include <sstream>
#include <fstream>

namespace HubPlatform {
namespace Opkg {

namespace {
constexpr const char BackupExtension[] = ".current";
}  // namespace

FeedsConfiguration::FeedsConfiguration( std::string feedsConfigurationPath )
: m_feedsConfigurationPath( std::move( feedsConfigurationPath ) )
, m_feedsConfigurationBackupPath( m_feedsConfigurationPath + BackupExtension )
, m_feedsSnapshot( readFeeds() ) {}

int FeedsConfiguration::getFeedsCount() const {
    return m_feedsSnapshot.size();
}

const Feed& FeedsConfiguration::getFeed( int idx ) const {
    static const Feed invalidFeed = {};
    if( idx < 0 || idx >= m_feedsSnapshot.size() )
        return invalidFeed;

    return m_feedsSnapshot[ idx ];
}

void FeedsConfiguration::addFeed( Feed feed ) {
    m_feedsSnapshot.emplace_back( std::move( feed ) );
}

Feed& FeedsConfiguration::takeFeed( int idx ) {
    static Feed invalidFeed;
    if( idx < 0 || idx >= m_feedsSnapshot.size() ) {
        invalidFeed = {};
        return invalidFeed;
    }

    return m_feedsSnapshot[ idx ];
}

void FeedsConfiguration::removeFeed( int idx ) {
    m_feedsSnapshot.erase( m_feedsSnapshot.begin() + idx );
}

void FeedsConfiguration::clearFeeds() {
    m_feedsSnapshot.clear();
}

void FeedsConfiguration::revert() {
    m_feedsSnapshot = readFeeds();
}

void FeedsConfiguration::commit() {
    writeFeeds( m_feedsSnapshot );
}

FeedsConfiguration::Feeds FeedsConfiguration::readFeeds() const {
    Feeds feeds;

    std::ifstream stream( m_feedsConfigurationPath );
    if( !stream.is_open() )
        return feeds;

    std::string line;
    while( getline( stream, line ) ) {
        auto feed = Feed::fromString( line );
        if( feed.isValid() )
            feeds.emplace_back( std::move( feed ) );
    }

    return feeds;
}

void FeedsConfiguration::writeFeeds( Feeds feeds ) {
    std::ofstream stream( m_feedsConfigurationPath, std::ofstream::trunc );
    if( !stream.is_open() )
        return;

    for( auto feed : feeds ) {
        auto line = Feed::toString( feed );
        if( !line.empty() )
            stream << line << '\n';
    }
    stream.flush();
}

bool FeedsConfiguration::removeBackup() {
    return filesystem::removeFile( m_feedsConfigurationBackupPath.c_str() );
}

bool FeedsConfiguration::tryBackup() {
    if( filesystem::isFileExist( m_feedsConfigurationBackupPath ) )
        return false;

    return filesystem::copyFile( m_feedsConfigurationPath, m_feedsConfigurationBackupPath );
}

bool FeedsConfiguration::tryRestore() {
    if( !filesystem::isFileExist( m_feedsConfigurationBackupPath ) )
        return false;

    if( !filesystem::copyFile( m_feedsConfigurationBackupPath, m_feedsConfigurationPath ) )
        return false;

    return removeBackup();
}

}  // namespace Opkg
}  // namespace HubPlatform