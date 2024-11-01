#include "update_manager/um_update_planner.h"
#include "update_manager/resources/um_resources.h"

#include "common_utils/addons/addon_version.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

#include <cstring>
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace UpdateManager {

UpdatePlanner::UpdatePlanner( const Opkg::IPackagesManager& packagesManager,
                              const Opkg::IFeedsUpdater& feedsUpdater,
                              Firmware::Version firmwareVersion )
: m_packagesManager( packagesManager )
, m_feedsUpdater( feedsUpdater )
, m_firmwareVersion( firmwareVersion ) {}

bool UpdatePlanner::isHubPlatformPackage( const std::string& name ) {
    return name == Resources::PackageNames::Firmware || isAddonPackage( name ) || isPluginPackage( name );
}

bool UpdatePlanner::isAddonPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::AddonPrefix,
                    strlen( Resources::PackageNames::AddonPrefix ) ) == 0;
}

bool UpdatePlanner::isPluginPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::PluginPrefix,
                    strlen( Resources::PackageNames::PluginPrefix ) ) == 0;
}

bool UpdatePlanner::isCustomPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::CustomPackagePrefix,
                    strlen( Resources::PackageNames::CustomPackagePrefix ) ) == 0;
}

std::unique_ptr< UpdatePlan > UpdatePlanner::createUpdatePlan( UpdateOperation operation ) {
    // TODO: create more optimal update plan
    //       - do not update/install packages when not required
    //       - do not try installing bundled packages after sysupgrade
    //       - uninstall bundled packages after sysupgrade if they were previously uninstalled manually
    //       - plan dependencies if they are not yet installed
    //       - do sysupgrade if non-firmware package changed, removed or added

    bool assumeHaveChanges = false;
    auto currentSource = m_feedsUpdater.getFeedsSource();
    auto currentSnapshot = m_feedsUpdater.getFeedsSnapshot();

    auto plan = std::make_unique< UpdatePlan >();
    // Special case for uninstalling package
    if( operation.operationType == UpdateOperationType::Uninstall ) {
        operation.source = currentSource;
        operation.snapshot = currentSnapshot;
        plan->steps.push_back( operation );

        auto dependentPackages = m_packagesManager.getDependantPackages( operation.packageSummary.name );
        for( auto& package : dependentPackages ) {
            if( !isHubPlatformPackage( package.name ) && !isCustomPackage( package.name ) )
                continue;

            plan->steps.push_back( { package, UpdateOperationType::Uninstall, currentSource, currentSnapshot } );
        }
        return plan;
    }

    std::unordered_map< std::string, std::string > availablePackageVersions;
    auto availablePackages = m_packagesManager.getAvailablePackages();
    std::string availableSystemVersion;
    for( auto& package : availablePackages ) {
        if( package.name == Resources::PackageNames::System ) {
            availableSystemVersion = package.version;
        }

        if( !isHubPlatformPackage( package.name ) && !isCustomPackage( package.name ) )
            continue;

        availablePackageVersions[ package.name ] = package.version;
    }

    // Check is we should actually do sysupgrade
    auto installedPackages = m_packagesManager.getInstalledPackages();
    std::string installedSystemVersion;
    for( auto& package : installedPackages ) {
        if( package.name == Resources::PackageNames::System ) {
            installedSystemVersion = package.version;
        }
    }

    auto firmwareUpdateOperationType = operation.operationType == UpdateOperationType::Sysupgrade ?
                                           UpdateOperationType::Sysupgrade :
                                           UpdateOperationType::Upgrade;

    if( installedSystemVersion.empty() || availableSystemVersion.empty() ||
        installedSystemVersion != availableSystemVersion ) {
        assumeHaveChanges = true;
        if( firmwareUpdateOperationType == UpdateOperationType::Upgrade ) {
            Log::warn( "Requested system version '" + availableSystemVersion + "' differs from current `" +
                       installedSystemVersion + "`, setting firmware update type to full" );
            firmwareUpdateOperationType = UpdateOperationType::Sysupgrade;
        }
    }

    if( firmwareUpdateOperationType == UpdateOperationType::Upgrade ) {
        if( !operation.source.empty() ) {
            Log::warn( "Requested switching to source " + operation.source + ", setting firmware update type to full" );
            assumeHaveChanges = true;
            firmwareUpdateOperationType = UpdateOperationType::Sysupgrade;
        }
    }

    if( firmwareUpdateOperationType == UpdateOperationType::Upgrade ) {
        for( auto& package : installedPackages ) {
            if( !isHubPlatformPackage( package.name ) && !isCustomPackage( package.name ) )
                continue;

            auto it = availablePackageVersions.find( package.name );
            if( it == availablePackageVersions.end() ) {
                Log::warn( cc::warn( "Installed package is not found in available packages: " ) + package.name );

                continue;
            }
            if( Common::BuildVersion( it->second ) < Common::BuildVersion( package.version ) ) {
                Log::warn( "Requested snapshot " + package.name + " version " + it->second + " is older then current " +
                           package.version + ", setting firmware update type to full" );
                firmwareUpdateOperationType = UpdateOperationType::Sysupgrade;
            }
        }
    }

    // Process firmware
    auto extensionsUpdateOperationType = UpdateOperationType::Upgrade;

    auto availableFirmwarePackageVersion = availablePackageVersions[ Resources::PackageNames::Firmware ];
    if( availableFirmwarePackageVersion != m_firmwareVersion.toString() ) {
        assumeHaveChanges = true;
    }

    if( ( availableFirmwarePackageVersion != m_firmwareVersion.toString() ) ||
        ( firmwareUpdateOperationType == UpdateOperationType::Sysupgrade ) ) {
        Opkg::PackageSummary availablePackage = { Resources::PackageNames::Firmware, availableFirmwarePackageVersion };
        plan->steps.push_back(
            { std::move( availablePackage ), firmwareUpdateOperationType, currentSource, currentSnapshot } );

        if( firmwareUpdateOperationType == UpdateOperationType::Sysupgrade ) {
            extensionsUpdateOperationType = UpdateOperationType::Install;
        }
    }

    // Process extensions
    for( auto& package : installedPackages ) {
        if( !isAddonPackage( package.name ) && !isPluginPackage( package.name ) && !isCustomPackage( package.name ) )
            continue;

        LOGGER_TRACE( cc::trace( "Update plan: Process installed extensions, package: " ) + package.name +
                      cc::trace( ", version: " ) + package.version );

        auto it = availablePackageVersions.find( package.name );
        if( it == availablePackageVersions.end() ) {
            assumeHaveChanges = true;
            LOGGER_TRACE_RAW( cc::trace( "Not found in available packages, uninstall" ) );
            plan->steps.emplace_back( package, UpdateOperationType::Uninstall, currentSource, currentSnapshot );
            continue;
        }

        if( package.version != it->second ) {
            LOGGER_DEBUG( cc::debug( "versions differ" ) );
            assumeHaveChanges = true;
        }

        if( ( package.version == it->second ) && ( firmwareUpdateOperationType != UpdateOperationType::Sysupgrade ) )
            continue;

        Opkg::PackageSummary availablePackage = { package.name, it->second };
        plan->steps.emplace_back( availablePackage, extensionsUpdateOperationType, currentSource, currentSnapshot );
    }

    // Do not forget to install initial extension
    if( operation.operationType == UpdateOperationType::Install ) {
        operation.source = currentSource;
        operation.snapshot = currentSnapshot;
        assumeHaveChanges = true;
        plan->steps.push_back( operation );
    }

    if( !assumeHaveChanges && ( firmwareUpdateOperationType == UpdateOperationType::Sysupgrade ) ) {
        plan->steps.clear();
    }

    return plan;
}

}  // namespace UpdateManager
}  // namespace HubPlatform
