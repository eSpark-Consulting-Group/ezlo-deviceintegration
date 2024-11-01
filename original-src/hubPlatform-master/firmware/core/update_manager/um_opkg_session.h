#pragma once

#include <utility>

#include "opkg_utils/opkg_manager.h"

namespace HubPlatform {
namespace UpdateManager {

class OpkgSessionException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class OpkgSessionAvailabilityFailed : public OpkgSessionException {
public:
    OpkgSessionAvailabilityFailed( int code, std::string output );
    int code() const {
        return m_code;
    }
    const std::string& output() const {
        return m_output;
    }

private:
    int m_code = 0;
    std::string m_output;
};

class OpkgSession final {
public:
    // DONE: EFWR-4028: Move feedsUpdater creation to client code
    explicit OpkgSession( Opkg::OpkgManager& opkgManager );
    OpkgSession( Opkg::OpkgManager& opkgManager, Opkg::IFeedUpdaterUPtr feedsUpdater );
    ~OpkgSession();

    bool updateFeeds( const std::string& source, int snapshot );
    void commit();

    bool verifyAvailablePackageVersion( Opkg::PackageSummary& packageSummary );

    bool updateAvailablePackages();
    bool checkAvailablePackages();

    const Opkg::IFeedsUpdater& getFeedsUpdater() const;

private:
    Opkg::IFeedsUpdater& takeFeedsUpdater();
    Opkg::IPackagesManager& takePackagesManager();
    bool tryUpdateAvailablePackages();

private:
    Opkg::OpkgManager& m_opkgManager;
    mutable Opkg::IFeedUpdaterUPtr m_feedsUpdater;
    mutable std::unique_ptr< Opkg::IPackagesManager > m_packagesManager;
    bool m_feedsUpdated{ false };
    bool m_opkgListsUpdated{ false };
};

}  // namespace UpdateManager
}  // namespace HubPlatform
