#pragma once

#include "opkg_utils/packages/packages_manager.h"
#include "opkg_utils/feeds/ifeeds_updater.h"

#include <fstream>

namespace HubPlatform {
namespace UpdateCommand {

class UpdateExecutor final {
public:
    UpdateExecutor( std::unique_ptr< Opkg::IPackagesManager > packagesManager,
                    std::unique_ptr< Opkg::IFeedsUpdater > feedsUpdater,
                    std::string journalFileName,
                    const std::string& logFileName,
                    bool isDaemon );

    bool run( bool singleStep = false );

private:
    int executeStep( const std::string& command, const std::string& packageName, const std::string& packageVersion );
    void changeStatus( std::fstream& journal, const char* newStatus );

    bool finishSysupgrade( const std::string& imageUrl, const std::string& packageVersion );
    int ensureAvailablePackagesUpToDate();

private:
    std::unique_ptr< Opkg::IPackagesManager > m_packagesManager;
    std::unique_ptr< Opkg::IFeedsUpdater > m_feedsUpdater;
    std::string m_journalFileName;
    std::ofstream m_log;
    bool m_availablePackagesOutdated;
    bool m_isDaemon;
};

}  // namespace UpdateCommand
}  // namespace HubPlatform
