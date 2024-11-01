#include "update_manager/um_service.h"
#include "update_manager/common/um_update_operation_type.h"
#include "update_manager/common/um_hub_reset_type.h"
#include "update_manager/resources/um_resources.h"
#include "update_manager/um_update_planner.h"
#include "update_manager/um_hub_resetter.h"

#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "extensions_manager/plugin_model/plugins_metedata_validator.h"
#include "extensions_manager/plugin_model/plugin_package_validator.h"
#include "extensions_manager/plugin_model/plugins_gateway_validator.h"
#include "extensions_manager/plugin_model/model_resources.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "common_utils/hub_state.h"
#include "common_utils/lock_helper.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "opkg_utils/opkg_manager.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "base_utils/md5.h"
#include "base_utils/utils.h"
#include "plugin_utils/plugin_utils.h"

#if( defined( __HUB_TYPE_G150__ ) )
#include "provision_data_utils/provision_data_manager.h"
#endif

#include "tiny/include/tiny/tiny_utils.h"

#include <fstream>
#include <cstring>
#include <chrono>

namespace HubPlatform {
namespace UpdateManager {

namespace {
constexpr const std::chrono::seconds UpdateProgressCheckPeriod( 1 );
constexpr const int RetriesCount = 2;
constexpr const std::chrono::seconds RetriesDelay( 10 );
constexpr const int StallChecks = 10;
constexpr const int PrepareProcessPercents = 20;

class CustomPluginError : public Errors::Exception {
public:
    using Errors::Exception::Exception;
};

class CustomPluginErrorWithMessage : public CustomPluginError {
public:
    CustomPluginErrorWithMessage( Errors::Error error, std::string message );

    const std::string& getMessage() const;

private:
    std::string m_message;
};

class CustomPluginCheckSumError : public CustomPluginError {
public:
    CustomPluginCheckSumError( Errors::Error error, std::string validCheckSum );

    const std::string& getValidCheckSum() const;

private:
    std::string m_validCheckSum;
};

CustomPluginCheckSumError::CustomPluginCheckSumError( Errors::Error error, std::string validCheckSum )
: CustomPluginError( error )
, m_validCheckSum( std::move( validCheckSum ) ) {}

const std::string& CustomPluginCheckSumError::getValidCheckSum() const {
    return m_validCheckSum;
}

CustomPluginErrorWithMessage::CustomPluginErrorWithMessage( Errors::Error error, std::string message )
: CustomPluginError( error )
, m_message( std::move( message ) ) {}

const std::string& CustomPluginErrorWithMessage::getMessage() const {
    return m_message;
}

}  // namespace

Service::Service()
: Services::Service( GroupsNames::UPDATE_MANAGER ) {
    reset();
}

void Service::setUp() {
    Services::Service::setUp();
    join( GroupsNames::HARDWARE_BROADCASTS );

    m_hardwareManager = std::make_unique< HardwareManager::RemoteInterface >( *this );

    m_journalFileName = Environment::getPersistentFolder() + "/" + Resources::UpdateJournalFileName;
    m_customPluginStorage = Environment::getCustomPluginsFolder() + "/";
    m_customPluginTemporaryStorage = Environment::getTempDataFolder() + "/automation_components/";

    filesystem::createAndClearFolder( m_customPluginTemporaryStorage );
    m_updateOperationsInProgress = 0;

    addTask( [this] { watchForUpdate(); }, true );
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    using MethodHandler = JsonObject ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { SOFTWARE_INFO_REQUEST, &Service::handleSoftwareInfoRequest },
        { GLOBAL_LOCK, &Service::handleLockRequest },
        { GLOBAL_UNLOCK, &Service::handleUnlockRequest },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( request );
    }

    return Services::Service::handleRequestFromBus( request, requestType );
}

void Service::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& /*sender*/ ) {
    using MethodHandler = void ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { SOFTWARE_UPDATE_PLAN_REQUEST, &Service::handleSoftwareUpdatePlanRequest },
    };
    static const MethodHandlersMap postponableMethodHandlers = {
        { SOFTWARE_UPDATE_PLAN_EXECUTE, &Service::handleSoftwareUpdatePlanExecute },
        { HUB_RESET_REQUEST, &Service::handleHubResetRequest },
        { LUUP_SWITCH, &Service::handleFirmwareLuupSwitch },
        { BOOTLOADER_UPDATE, &Service::handleBootloaderUpdate },
        { RECOVERY_PARTITION_UPDATE, &Service::handleRecoveryPartitionUpdate },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
    const auto pit = postponableMethodHandlers.find( requestType );
    if( pit != postponableMethodHandlers.end() ) {
        const auto handler = pit->second;
        CommonUtils::LockHelper::global().postponeIfLocked( [=]() { ( this->*handler )( json ); } );
    }
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventFromBus( message, sender );
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleEventFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgSubClass ) {
    case BUTTON_FACTORY_RESET: {
        HubResetter hubResetter( *m_hardwareManager );
#if( defined( __HUB_TYPE_G150__ ) )
        if( !ProvisionDataUtils::getManager().getProvisionData().isValid() ) {
            handleFirmwareLuupSwitch( {} );
        }
        else
#endif
        {
            hubResetter.reset( HubResetType::Factory );
        }
        break;
    }
    case BUTTON_SOFT_RESET: {
        HubResetter hubResetter( *m_hardwareManager );
        hubResetter.reset( HubResetType::Soft );
        break;
    }

    default:
        break;
    }
}

JsonObject Service::handleSoftwareInfoRequest( const JsonObject& json ) {
    CommonUtils::HubState hubState;
    hubState.firmware = FirmwareInfoHelper::getFirmwareVersion();

    auto packagesManager = m_opkgManager.createPackagesManager();
    auto installedPackages = packagesManager->getInstalledPackages();
    for( auto& package : installedPackages ) {
        if( isAddonPackage( package.name ) ) {
            hubState.addons.push_back( { package.name.substr( strlen( Resources::PackageNames::AddonPrefix ) ),
                                         CommonUtils::AddonVersion( package.version ) } );
        }

        if( isPluginPackage( package.name ) ) {
            hubState.plugins.push_back( { package.name.substr( strlen( Resources::PackageNames::PluginPrefix ) ),
                                          CommonUtils::PluginVersion( package.version ) } );
        }

        if( isCustomPackage( package.name ) ) {
            hubState.customPackages.push_back(
                { package.name.substr( strlen( Resources::PackageNames::CustomPackagePrefix ) ),
                  CommonUtils::PluginVersion( package.version ) } );
        }
    }

    JsonObject response = hubState.toJson();
    return response;
}

JsonObject Service::handleLockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.tryLock( json );
    return lockHelper.getResponse();
}

JsonObject Service::handleUnlockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.unlock( json );
    return lockHelper.getResponse();
}

void Service::handleSoftwareUpdatePlanRequest( const JsonObject& json ) {
    auto updateOperation = UpdateOperation::fromJson( json[ "operation" ] );
    auto operationId = json_get_str( json, "operationId" );

    JsonObject data;

    auto updatePlan = requestUpdatePlan( std::move( updateOperation ) );
    if( !updatePlan ) {
        setError( data, Errors::UPDATE_OPERATIONS_ERROR );
        json_add_str( data, "operationId", operationId );
        Log::error( "Could not create an update plan" );
    }
    else {
        data = updatePlan->toJson();
        json_add_null( data, "error" );
        json_add_str( data, "operationId", operationId );
    }

    Services::Message message = { "ui_service", EVENT, PLANNING_SOFTWARE_UPDATE, data };
    send( message );
}

void Service::handleSoftwareUpdatePlanExecute( const JsonObject& json ) {
    auto updatePlan = std::make_unique< UpdatePlan >( UpdatePlan::fromJson( json ) );
    auto operationId = json_get_str( json, "operationId" );

    m_operationId = operationId;

    LOGGER_INFO_RAW( cc::info( "Got request to execute update plan" ) );

    handleUpdateOperationStart();
    sendProgressStatus( m_operationId, "started", 0 );

    executeUpdatePlan( std::move( updatePlan ), operationId );
}

void Service::handleFirmwareLuupSwitch( const JsonObject& json ) {
    HubResetter hubResetter( *m_hardwareManager );
    hubResetter.switchToLuup();
}

void Service::handleBootloaderUpdate( const JsonObject& json ) {
    using namespace HubPlatform::HardwareManager;
    Log::debug( "Request to update bootloader" );

    m_hardwareManager->onProcessRunning( HubState::Process::BootloaderUpdate );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon --update_bootloader" ).c_str() );

    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
    m_hardwareManager->onProcessStopped( HubState::Process::BootloaderUpdate, HubState::ProcessResult::Fail );
}

void Service::handleRecoveryPartitionUpdate( const JsonObject& json ) {
    using namespace HubPlatform::HardwareManager;
    Log::debug( "Request to update recovery partition" );

    m_hardwareManager->onProcessRunning( HubState::Process::RecoveryPartitionUpdate );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon --update_recovery_partition" ).c_str() );

    createPostponedTask( [this]( Services::PostponedTask& ) {
        m_hardwareManager->onProcessStopped( HubState::Process::RecoveryPartitionUpdate,
                                             HubState::ProcessResult::Fail );
    } )
        ->start( std::chrono::minutes( 10 ), false );
}

void Service::handleHubResetRequest( const JsonObject& json ) {
    auto resetType = hubResetTypeFromJson( json );
    HubResetter hubResetter( *m_hardwareManager );
    hubResetter.reset( resetType );
}

bool Service::verifyInstalledPackageVersion( const Opkg::PackageSummary& packageSummary ) {
    using namespace HubPlatform::HardwareManager;
    auto packagesManager = m_opkgManager.createPackagesManager();

    if( packageSummary.version.empty() ) {
        auto isProperPackage = [&]( const Opkg::PackageSummary& package ) {
            return package.name == packageSummary.name;
        };

        auto packages = packagesManager->getInstalledPackages();
        auto it = std::find_if( packages.begin(), packages.end(), isProperPackage );
        if( it == packages.end() ) {
            Log::info( "Package " + packageSummary.name + " is not installed" );
            return false;
        }
    }
    else {
        auto isProperPackage = [&]( const Opkg::PackageSummary& package ) {
            return ( package.name == packageSummary.name ) && ( package.version == packageSummary.version );
        };

        auto packages = packagesManager->getInstalledPackages();
        auto it = std::find_if( packages.begin(), packages.end(), isProperPackage );
        if( it == packages.end() ) {
            Log::info( "Requested " + packageSummary.name + " version is not installed" );
            return false;
        }
    }

    return true;
}

bool Service::isAddonPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::AddonPrefix,
                    strlen( Resources::PackageNames::AddonPrefix ) ) == 0;
}

bool Service::isPluginPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::PluginPrefix,
                    strlen( Resources::PackageNames::PluginPrefix ) ) == 0;
}

bool Service::isCustomPackage( const std::string& name ) {
    return strncmp( name.c_str(),
                    Resources::PackageNames::CustomPackagePrefix,
                    strlen( Resources::PackageNames::CustomPackagePrefix ) ) == 0;
}

std::unique_ptr< UpdatePlan > Service::requestUpdatePlan( UpdateOperation operation ) {
    Log::info(
        cc::info( "Got request to create update plan for " ) +
        cc::note( updateOperationTypeToString( operation.operationType ) ) + " " +
        cc::note( operation.packageSummary.name ) +
        ( operation.packageSummary.version.empty() ?
              std::string() :
              cc::info( "(" ) + cc::note( operation.packageSummary.version ) + cc::info( ")" ) ) +
        ( operation.source.empty() ? std::string() : cc::info( " from " ) + cc::u( operation.source ) ) +
        ( operation.snapshot == 0 ? std::string() : cc::info( " from snapshot " ) + cc::num10( operation.snapshot ) ) );

    cleanUnexpectedFolder();

    if( filesystem::isFileExist( m_journalFileName ) ) {
        std::ifstream journal( m_journalFileName );
        if( journal.is_open() && journal.peek() != std::ifstream::traits_type::eof() ) {
            Log::error( cc::error( "Update process is in in progress" ) );
            return {};
        }
    }
    auto sessionFeedsType = selectFeedType( operation.source, operation.snapshot );

    OpkgSession opkgSession( m_opkgManager, m_opkgManager.createFeedsUpdater( sessionFeedsType ) );

    // Step 4: Finally, update feeds with correctly tuned feedUpdated
    if( !opkgSession.updateFeeds( operation.source, operation.snapshot ) ) {
        Log::error( cc::error( "Can not update feeds" ) );
        return {};
    }

    // Check package status or availability, fix operation if possible
    switch( operation.operationType ) {
    case UpdateOperationType::Install:
        if( operation.packageSummary.name == Resources::PackageNames::Firmware ) {
            Log::error( "Can not install firmware, only upgrade or sysupgrade" );
            return {};
        }
        if( !opkgSession.verifyAvailablePackageVersion( operation.packageSummary ) ) {
            return {};
        }
        if( verifyInstalledPackageVersion( operation.packageSummary ) ) {
            Log::warn( "Requested package " + operation.packageSummary.name +
                       " is already installed, doing upgrade instead" );
            operation.operationType = UpdateOperationType::Upgrade;
        }
        break;
    case UpdateOperationType::Sysupgrade:
        if( operation.packageSummary.name != Resources::PackageNames::Firmware ) {
            Log::error( "Can sysupgrade only firmware" );
            return {};
        }
        if( !opkgSession.verifyAvailablePackageVersion( operation.packageSummary ) ) {
            return {};
        }
        break;
    case UpdateOperationType::Upgrade:
        if( !opkgSession.verifyAvailablePackageVersion( operation.packageSummary ) ) {
            return {};
        }
        if( !verifyInstalledPackageVersion( { operation.packageSummary.name, "" } ) ) {
            Log::warn( "Requested package " + operation.packageSummary.name +
                       " is not yet installed, doing install instead" );
            operation.operationType = UpdateOperationType::Install;
        }
        break;
    case UpdateOperationType::Uninstall:
        if( operation.packageSummary.name == Resources::PackageNames::Firmware ) {
            Log::error( "Can not uninstall firmware, only upgrade or sysupgrade" );
            return {};
        }
        if( !verifyInstalledPackageVersion( operation.packageSummary ) ) {
            return {};
        }
        break;
    }

    auto packagesManager = m_opkgManager.createPackagesManager();
    UpdatePlanner planner( *packagesManager, opkgSession.getFeedsUpdater(), FirmwareInfoHelper::getFirmwareVersion() );
    auto plan = planner.createUpdatePlan( operation );

    return plan;
}

std::string Service::getUpdatePlanSysupgradeVersion( const JsonObject& plan ) {
    auto updateOperation = UpdateOperation::fromJson( plan );
    auto updatePlan = requestUpdatePlan( updateOperation );
    if( updatePlan ) {
        for( auto& step : updatePlan->steps ) {
            if( step.operationType == UpdateOperationType::Sysupgrade ) {
                return step.packageSummary.version;
            }
        }
    }

    return "";
}

void Service::executeUpdatePlan( std::unique_ptr< UpdatePlan > updatePlan, const std::string& operationId ) {
    using namespace HubPlatform::HardwareManager;

    // Should have at least one step
    if( !updatePlan || !updatePlan->isValid() ) {
        Log::error( "Invalid update plan" );
        handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
        return;
    }

    auto& firstOperation = updatePlan->steps.front();

    // custom plugins are distributed as plain archive files, so handle them differently
    if( firstOperation.componentType == UpdateComponentType::CustomPlugin ) {
        executeCustomPluginUpdateExecute( *updatePlan );
        return;
    }

    // Check steps consistency
    for( auto& operation : updatePlan->steps ) {
        if( ( operation.snapshot != firstOperation.snapshot ) || ( operation.source != firstOperation.source ) ) {
            Log::error( "Can not update feeds" );
            handleUpdateOperationFailed( m_operationId, Errors::BAD_PARAMS );
            return;
        }
    }

    auto sessionFeedsType = selectFeedType( firstOperation.source, firstOperation.snapshot );

    auto feedsUpdater = m_opkgManager.createFeedsUpdater( sessionFeedsType );

    // Update feeds
    if( !feedsUpdater->update( firstOperation.source, firstOperation.snapshot ) ) {
        Log::error( "Can not update feeds" );
        handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
        return;
    }

    OpkgSession opkgSession( m_opkgManager );
    // Check all required packages are available
    for( auto& operation : updatePlan->steps ) {
        switch( operation.operationType ) {
        case UpdateOperationType::Install:
        case UpdateOperationType::Upgrade:
            if( !opkgSession.verifyAvailablePackageVersion( operation.packageSummary ) ) {
                handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
                return;
            }
            break;
        case UpdateOperationType::Uninstall:
            if( !verifyInstalledPackageVersion( operation.packageSummary ) ) {
                handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
                return;
            }
            break;
        }
    }

    cleanUnexpectedFolder();

    if( !filesystem::isFileExist( m_journalFileName ) ) {
        cleanJournal();
    }

    std::ofstream journal( m_journalFileName, std::ofstream::app );
    if( !journal.is_open() ) {
        Log::error( cc::error( "Error writing entries to update journal" ) );
        handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
        return;
    }
    if( journal.tellp() ) {
        Log::error( cc::error( "Another update process is in in progress" ) );
        handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
        return;
    }

    for( auto& step : updatePlan->steps ) {
        auto version =
            step.packageSummary.version.empty() ? std::string( Resources::LatestVersion ) : step.packageSummary.version;
        std::string name = step.packageSummary.name;
        if( ( step.operationType == UpdateOperationType::Sysupgrade ) &&
            ( step.packageSummary.name == Resources::PackageNames::Firmware ) ) {
            name = feedsUpdater->generateImageUrl();

            auto packageVersion = step.packageSummary.version;
            if( packageVersion.empty() || version != packageVersion ) {
                auto sysupgradeVersion = getUpdatePlanSysupgradeVersion( step.toJson() );
                if( !sysupgradeVersion.empty() ) {
                    version = sysupgradeVersion;
                }
            }
        }
        Log::info( cc::info( "Add line to journal: name " ) + cc::info( name ) + cc::info( " version " ) +
                   cc::info( version ) );

        journal << updateOperationTypeToString( step.operationType ) << " " << name << " " << version << " "
                << Resources::Statuses::Scheduled << std::endl;
    }

    journal.flush();

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_BEEWAVE__ )
    // Data partition is too small to be sure there will be enough
    // free space to store everything required for update
    // Also it is part of subsequent restore process witch will require
    // separate implementation for such controller(s).
#else
    if( !predownload( *updatePlan, opkgSession ) ) {
        handleUpdateOperationFailed( m_operationId, Errors::CANT_DOWNLOAD_FIRMWARE );
        cleanJournal();
        cleanCache();
        return;
    }
#endif
    Log::info( cc::info( "Prepared for update process: " ) + cc::num10( PrepareProcessPercents ) + cc::info( " %" ) );
    sendProgressStatus( m_operationId, "process", PrepareProcessPercents );

    notifyPluginsUnavailable( *updatePlan );

    system( ( Environment::getBinariesFolder() + "/updatecmd --daemon" ).c_str() );
}

bool Service::checkCustomPluginFile( const std::string& filePath ) {
    ExtensionsManager::RemoteInterface extensionManagerInterface( *this );
    std::string componentName;
    std::string relativeFilePath;

    std::tie( componentName, relativeFilePath ) = extensionManagerInterface.splitFileId( filePath );
    if( componentName.empty() || relativeFilePath.empty() ) {
        return false;
    }

    return filesystem::isFileExist( m_customPluginTemporaryStorage + componentName + "/" + relativeFilePath );
}

bool Service::isCustomPluginOperationRunning( const std::string& pluginId ) {
    std::unique_lock< std::mutex > lock( m_updateCustomPluginOperationsMtx );

    auto it = m_updateCustomPluginOperations.find( pluginId );
    if( it != m_updateCustomPluginOperations.end() ) {
        return true;
    }
    return false;
}

void Service::setCustomPluginOperationRunning( const std::string& pluginId ) {
    std::unique_lock< std::mutex > lock( m_updateCustomPluginOperationsMtx );

    m_updateCustomPluginOperations.emplace( pluginId );
}

void Service::setCustomPluginOperationRunningNoLock( const std::string& pluginId ) {
    m_updateCustomPluginOperations.emplace( pluginId );
}

void Service::unsetCustomPluginOperationRunning( const std::string& pluginId ) {
    std::unique_lock< std::mutex > lock( m_updateCustomPluginOperationsMtx );

    m_updateCustomPluginOperations.erase( pluginId );
}

void Service::deployCustomPlugin( const std::string& dearchivePath,
                                  const std::string& componentPath,
                                  const std::string& componentName ) {
    std::string errorPath;
    std::string errorMessage;
    if( !filesystem::createAndClearFolder(
            componentPath,
            [&errorPath, &errorMessage]( int err, const std::string& message, const std::string& path ) {
                errorMessage = message;
                errorPath = path;
            } ) ||
        !filesystem::copyFolder(
            dearchivePath,
            componentPath,
            [&errorPath, &errorMessage]( int err, const std::string& message, const std::string& path ) {
                errorMessage = message;
                errorPath = path;
            } ) ) {
        std::size_t pos = errorPath.find( std::string( "/" ) + componentName + "/" );
        if( pos != std::string::npos ) {
            errorPath = std::string( "HUB:" ) + errorPath.substr( pos + 1 );
        }
        auto message = errorMessage + "," + " failed: [" + errorPath + "]";
        LOGGER_DEBUG_RAW( cc::debug( "Failed to deploy a custom plugin " ) + cc::note( componentName ) );
        throw CustomPluginErrorWithMessage( Errors::CUSTOM_PLUGIN_FAILED_TO_DEPLOY, errorMessage );
    }
}

void Service::onCustomPluginDownloaded( const UpdateOperation& updateOperation, const std::string& operationId ) {
    ExtensionsManager::RemoteInterface extensionManagerInterface( *this );

    std::string dearchivePath;
    std::string archivePath( m_customPluginTemporaryStorage + updateOperation.packageSummary.name + ".tgz" );
    std::string componentPath( m_customPluginStorage + updateOperation.packageSummary.name );

    try {
        auto fileSize = filesystem::getFileSize( archivePath );
        if( 0 == fileSize ) {
            LOGGER_DEBUG_RAW( cc::error( "Custom plugin archive file has zero length" ) );
            throw CustomPluginError( Errors::CUSTOM_PLUGIN_ARCHIVE_EMPTY );
        }

        std::ifstream archiveFile( archivePath );
        Md5 md5;
        md5.append( archiveFile );

        const auto checkSum = md5.getHex();
        if( checkSum != updateOperation.componentHash && toLower( checkSum ) != updateOperation.componentHash ) {
            LOGGER_DEBUG_RAW( cc::debug( "Invalid checksum for a custom plugin " ) +
                              cc::note( updateOperation.packageSummary.name ) + cc::debug( ", should be " ) +
                              cc::note( checkSum ) );
            throw CustomPluginCheckSumError( Errors::CUSTOM_PLUGIN_INVALID_CHECKSUM, checkSum );
        }

        LOGGER_INFO_RAW( cc::info( "Unpack a custom plugin " ) + cc::note( updateOperation.packageSummary.name ) +
                         cc::info( "..." ) );

        dearchivePath = filesystem::createRandomFolder( m_customPluginTemporaryStorage );
        System::untarArchive( archivePath, dearchivePath );

        // check custom plugin metadata
        LOGGER_INFO_RAW( cc::info( "Validate a custom plugin " ) + cc::note( updateOperation.packageSummary.name ) );

        ExtensionsManager::PluginPackageValidator pluginPackageValidator( archivePath,
                                                                          updateOperation.packageSummary.name );
        pluginPackageValidator.validate();

        if( updateOperation.operationType == UpdateOperationType::Upgrade ) {
            // TODO: rollback a failed upgrade operation
            LOGGER_INFO_RAW( cc::info( "Upgrade a custom plugin " ) + cc::note( updateOperation.packageSummary.name ) );

            sendProgressStatus( operationId, "process", 60 );

            auto response = extensionManagerInterface.pauseCustomPlugin( updateOperation.packageSummary.name );
            if( hasError( response ) ) {
                throw CustomPluginErrorWithMessage( Errors::PLUGIN_INSTALL_FAILED,
                                                    "Failed to upgrade plugin: " + getErrorMessage( response ) );
            }
            extensionManagerInterface.removePluginData( updateOperation.packageSummary.name );
            filesystem::deleteFolder( componentPath );
        }
        else {
            LOGGER_INFO_RAW( cc::info( "Install a custom plugin " ) + cc::note( updateOperation.packageSummary.name ) );
        }

        sendProgressStatus( operationId, "process", 80 );

        // deploy & run a custom plugin
        deployCustomPlugin( dearchivePath, componentPath, updateOperation.packageSummary.name );
        {
            std::string configFilePath =
                dearchivePath + "/" + ::ExtensionsManager::Resources::PluginFiles::ConfigFileName;
            std::string interfaceFilePath =
                dearchivePath + "/" + ::ExtensionsManager::Resources::PluginFiles::InterfaceFileName;
            std::function< bool( std::string ) > isFileExist = [this, dearchivePath]( const std::string& fileId ) {
                std::string componentName;
                std::string relativeFilePath;
                ExtensionsManager::PluginUtils pluginUtils;

                std::tie( componentName, relativeFilePath ) = pluginUtils.splitScriptId( fileId );
                if( componentName.empty() || relativeFilePath.empty() ) {
                    return false;
                }

                return filesystem::isFileExist( dearchivePath + "/" + relativeFilePath );
            };
            std::ifstream configFile( configFilePath );
            std::ifstream interfaceFile( interfaceFilePath );

            extensionManagerInterface.setPluginData( configFile, interfaceFile, isFileExist );
        }
        auto response = extensionManagerInterface.registerCustomPlugin( updateOperation.packageSummary.name );
        if( hasError( response ) ) {
            throw CustomPluginErrorWithMessage( Errors::PLUGIN_INSTALL_FAILED,
                                                "Failed to register plugin: " + getErrorMessage( response ) );
        }

        std::string operationName =
            ( updateOperation.operationType == UpdateOperationType::Upgrade ) ? "upgraded" : "installed";
        LOGGER_INFO_RAW( cc::info( "A custom plugin was " + operationName + ", name: " ) +
                         cc::note( updateOperation.packageSummary.name ) );

        handleUpdateOperationSuccessful( operationId );
    }
    catch( const CustomPluginErrorWithMessage& ex ) {
        handleUpdateOperationFailedWithMessage( operationId, ex.getError(), ex.getMessage() );
    }
    catch( const CustomPluginCheckSumError& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, invalid checksum "
                                     "for " ) +
                          cc::note( updateOperation.packageSummary.name ) + cc::error( " plugin, should be: " ) +
                          cc::note( ex.getValidCheckSum() ) );
        handleUpdateOperationFailedWithMessage( operationId, ex.getError(), ex.what() );
    }
    catch( const ::ExtensionsManager::PluginsGatewayValidatorException& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, gateway data error: " ) +
                          cc::note( ex.what() ) );

        const auto errorMessage = ex.getPath().toString() + ": " + ex.what();
        handleUpdateOperationFailedWithMessage( operationId, Errors::CUSTOM_PLUGIN_GATEWAY_INVALID, errorMessage );
    }
    catch( const Errors::Exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, internal error "
                                     "for " ) +
                          cc::note( updateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailedWithMessage( operationId, ex.getError(), ex.what() );
    }
    catch( const ::ExtensionsManager::PluginsMetadataValidatorException& ex ) {
        const auto errorMessage = ex.getPath().toString() + ": " + ex.what();
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, invalid metadata "
                                     "for " ) +
                          cc::note( updateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                          cc::note( errorMessage ) );
        handleUpdateOperationFailedWithMessage( operationId, Errors::CUSTOM_PLUGIN_INVALID_METADATA, errorMessage );
    }
    catch( const System::SystemError& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, system error happened "
                                     "for " ) +
                          cc::note( updateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailedWithMessage( operationId, Errors::SYSTEM_ERROR, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, reason: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailedWithMessage( operationId, Errors::SYSTEM_ERROR, ex.what() );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins" ) );
        handleUpdateOperationFailed( operationId );
    }

    filesystem::deleteFolder( dearchivePath );
    filesystem::deleteFile( archivePath );
    unsetCustomPluginOperationRunning( updateOperation.packageSummary.name );
}

void Service::onCustomPluginFailedToDownload( const UpdateOperation& updateOperation,
                                              const std::string& operationId,
                                              const std::string& errorDescription ) {
    std::string archivePath( m_customPluginTemporaryStorage + updateOperation.packageSummary.name + ".tgz" );

    LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, internal error for " ) +
                      cc::note( updateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                      cc::note( errorDescription ) );

    handleUpdateOperationFailedWithMessage( operationId, Errors::CUSTOM_PLUGIN_FAILED_TO_DOWNLOAD, errorDescription );

    filesystem::deleteFile( archivePath );
    unsetCustomPluginOperationRunning( updateOperation.packageSummary.name );
}

void Service::executeCustomPluginUpdateExecute( const UpdatePlan& updatePlan ) {
    UpdateOperation currentUpdateOperation;
    auto operationId = m_operationId;

    try {
        // TODO: support update.execute with multiple targets (clean-up during a failure)
        if( updatePlan.steps.size() > 1 ) {
            LOGGER_DEBUG_RAW(
                cc::debug( "We don't support multiple targets in custom plugin update.execute request" ) );
            throw CustomPluginError( Errors::NOT_SUPPORTED );
        }

        for( const auto& updateOperation : updatePlan.steps ) {
            ExtensionsManager::RemoteInterface extensionManagerInterface( *this );
            currentUpdateOperation = updateOperation;

            std::string archivePath( m_customPluginTemporaryStorage + updateOperation.packageSummary.name + ".tgz" );
            std::string componentPath( m_customPluginStorage + updateOperation.packageSummary.name );

            if( updateOperation.operationType == UpdateOperationType::Uninstall ||
                updateOperation.operationType == UpdateOperationType::Upgrade ) {
                if( !extensionManagerInterface.isPluginData( updateOperation.packageSummary.name ) ) {
                    LOGGER_DEBUG_RAW( cc::debug( "The custom plugin " ) +
                                      cc::note( updateOperation.packageSummary.name ) +
                                      cc::debug( " isn't installed" ) );
                    throw CustomPluginErrorWithMessage( Errors::CUSTOM_PLUGIN_NO_INSTALLED,
                                                        "Couldn't load plugin metadata" );
                }
            }
            if( updateOperation.operationType == UpdateOperationType::Install ) {
                if( extensionManagerInterface.isPluginData( updateOperation.packageSummary.name ) ) {
                    LOGGER_DEBUG_RAW( cc::debug( "The custom plugin " ) +
                                      cc::note( updateOperation.packageSummary.name ) +
                                      cc::debug( " is already installed" ) );
                    throw CustomPluginError( Errors::CUSTOM_PLUGIN_ALREADY_INSTALLED );
                }
            }
            if( updateOperation.operationType == UpdateOperationType::Install ||
                updateOperation.operationType == UpdateOperationType::Upgrade ) {
                if( isCustomPluginOperationRunning( updateOperation.packageSummary.name ) ) {
                    LOGGER_DEBUG_RAW( cc::debug( "The custom plugin operation is running for a " ) +
                                      cc::note( updateOperation.packageSummary.name ) +
                                      cc::debug( " plugin already" ) );
                    throw CustomPluginError( Errors::CUSTOM_PLUGIN_OPERATION_IS_RUNNING );
                }
            }

            if( updateOperation.operationType == UpdateOperationType::Uninstall ) {
                LOGGER_INFO_RAW( cc::info( "Uninstall a custom plugin " ) +
                                 cc::note( updateOperation.packageSummary.name ) );

                sendProgressStatus( operationId, "process", 20 );

                Json response;
                response = extensionManagerInterface.unregisterCustomPlugin( updateOperation.packageSummary.name );
                if( hasError( response ) ) {
                    throw CustomPluginErrorWithMessage( Errors::PLUGIN_UNINSTALL_FAILED,
                                                        "Failed to unregister plugin: " + getErrorMessage( response ) );
                }
                extensionManagerInterface.removePluginData( updateOperation.packageSummary.name );
                filesystem::deleteFolder( componentPath );

                LOGGER_INFO_RAW( cc::info( "A custom plugin was uninstalled, name: " ) +
                                 cc::note( updateOperation.packageSummary.name ) );

                handleUpdateOperationSuccessful( operationId );
            }
            else {
                sendProgressStatus( operationId, "process", 20 );

                LOGGER_INFO_RAW( cc::info( "Download a custom plugin " ) +
                                 cc::note( updateOperation.packageSummary.name ) + cc::info( "..." ) );

                std::unique_lock< std::mutex > lock( m_updateCustomPluginOperationsMtx );

                m_customPluginDownloader.downloadFileAsync(
                    updateOperation.source,
                    archivePath,
                    [this, updateOperation, operationId]() {
                        onCustomPluginDownloaded( updateOperation, operationId );
                    },
                    [this, updateOperation, operationId]( const std::string& errorDescription ) {
                        onCustomPluginFailedToDownload( updateOperation, operationId, errorDescription );
                    } );

                setCustomPluginOperationRunningNoLock( updateOperation.packageSummary.name );
                lock.unlock();
            }
        }
    }
    catch( const Errors::Exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, internal error "
                                     "for " ) +
                          cc::note( currentUpdateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailedWithMessage( operationId, ex.getError(), ex.what() );
    }
    catch( const System::SystemError& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, system error happened "
                                     "for " ) +
                          cc::note( currentUpdateOperation.packageSummary.name ) + cc::error( " plugin, details: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailedWithMessage( operationId, Errors::SYSTEM_ERROR, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins, reason: " ) +
                          cc::note( ex.what() ) );
        handleUpdateOperationFailed( operationId );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to execute an update plan for custom plugins" ) );
        handleUpdateOperationFailed( operationId );
    }
}

void Service::handleUpdateOperationStart() {
    using namespace HubPlatform::HardwareManager;

    std::lock_guard< std::mutex > lock( m_updateOperationsInProgressMtx );

    if( !m_updateOperationsInProgress ) {
        m_hardwareManager->onProcessRunning( HubState::Process::FirmwareUpdate );
        CommonUtils::LockHelper::global().onTransactionRunning(
            static_cast< int >( HubState::Process::FirmwareUpdate ) );
    }

    m_updateOperationsInProgress++;
}
void Service::handleUpdateOperationFailed( const std::string& operationId, Errors::Error error ) {
    handleUpdateOperationFailedWithMessage( operationId, error, "" );
}
void Service::handleUpdateOperationFailedWithMessage( const std::string& operationId,
                                                      Errors::Error error,
                                                      const std::string& message ) {
    using namespace HubPlatform::HardwareManager;
    std::lock_guard< std::mutex > lock( m_updateOperationsInProgressMtx );

    sendProgressError( operationId, error, message );
    getDBClient().remove( HaUpdateManagerPendingUpdate,
                          JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::PendingUpdate ) );

    m_hardwareManager->onProcessStopped( HubState::Process::FirmwareUpdate, HubState::ProcessResult::Fail );
    CommonUtils::LockHelper::global().onTransactionStopped( static_cast< int >( HubState::Process::FirmwareUpdate ) );

    m_updateOperationsInProgress--;
    if( m_updateOperationsInProgress ) {
        m_hardwareManager->onProcessRunning( HubState::Process::FirmwareUpdate );
        CommonUtils::LockHelper::global().onTransactionRunning(
            static_cast< int >( HubState::Process::FirmwareUpdate ) );
    }

    m_opkgManager.createFeedsUpdater()->resetToCurrent();
    reset();
}

void Service::handleUpdateOperationSuccessful( const std::string& operationId ) {
    using namespace HubPlatform::HardwareManager;

    std::lock_guard< std::mutex > lock( m_updateOperationsInProgressMtx );

    sendProgressStatus( operationId, "finished", 100 );

    m_updateOperationsInProgress--;
    if( !m_updateOperationsInProgress ) {
        m_hardwareManager->onProcessStopped( HubState::Process::FirmwareUpdate, HubState::ProcessResult::Success );
        CommonUtils::LockHelper::global().onTransactionStopped(
            static_cast< int >( HubState::Process::FirmwareUpdate ) );
    }
}

bool Service::predownload( const UpdatePlan& updatePlan, OpkgSession& opkgSession ) {
    opkgSession.updateAvailablePackages();

    if( !filesystem::deleteFolder( Resources::CacheDirectories::Update ) ||
        !filesystem::createFolder( Resources::CacheDirectories::Update ) ||
        !filesystem::createFolder( Resources::CacheDirectories::Packages ) ||
        !filesystem::createFolder( Resources::CacheDirectories::SystemImage ) ||
        !filesystem::createFolder( Resources::CacheDirectories::Distfeeds ) ||
        !filesystem::copyFolder( "/tmp/opkg-lists", Resources::CacheDirectories::OpkgLists ) ||
        !filesystem::copyFile( "/etc/opkg/distfeeds.conf",
                               Resources::CacheDirectories::Distfeeds + std::string( "/distfeeds.conf" ) ) ) {
        return false;
    }

    std::string output;
    int result = 0;

    std::unordered_map< std::string, std::string > installedPackageVersions;
    auto installedPackages = m_opkgManager.createPackagesManager()->getInstalledPackages();
    for( auto& package : installedPackages ) {
        installedPackageVersions[ package.name ] = package.version;
    }

    // Download system image or firmware packages
    std::vector< std::string > packages;
    for( auto& step : updatePlan.steps ) {
        if( step.packageSummary.name == Resources::PackageNames::Firmware ) {
            if( step.operationType == UpdateOperationType::Sysupgrade ) {
                auto url = opkgSession.getFeedsUpdater().generateImageUrl();
                Log::info( cc::info( "Downloading system image" ) );
                for( int attempt = 0; attempt < RetriesCount + 1; ++attempt ) {
                    if( attempt != 0 ) {
                        Log::warn( cc::warn( "Retry #" ) + cc::num10( attempt ) + cc::warn( " in " ) +
                                   cc::num10( RetriesDelay.count() ) + cc::warn( " seconds" ) );
                        std::this_thread::sleep_for( RetriesDelay );
                    }
                    std::tie( output, result ) = m_opkgManager.createPackagesManager()->downloadImage(
                        url, Resources::CacheDirectories::SystemImage );

                    if( result == 0 ) {
                        break;
                    }
                    else if( attempt <= RetriesCount ) {
                        Log::debug( cc::debug( "Download failed: " ) + cc::note( output ) );
                    }
                }

                if( result != 0 ) {
                    Log::error( cc::error( "Failed to download system image: " ) + cc::note( output ) );
                    return false;
                }
            }
            else {
                Log::info( cc::info( "Downloading firmware packages" ) );
                for( int i = 0; i < RetriesCount + 1; ++i ) {
                    if( i != 0 ) {
                        Log::warn( cc::warn( "Retry #" ) + cc::num10( i ) + cc::warn( " in " ) +
                                   cc::num10( RetriesDelay.count() ) + cc::warn( " seconds" ) );
                        std::this_thread::sleep_for( RetriesDelay );
                    }
                    std::tie( output, result ) = m_opkgManager.createPackagesManager()->downloadPackage(
                        step.packageSummary.name, Resources::CacheDirectories::Packages, true );
                    Log::debug( output );

                    if( result == 0 )
                        break;
                }

                if( result != 0 ) {
                    Log::error( cc::error( "Failed to download firmware packages" ) );
                    return false;
                }
            }
        }
        else {
            if( step.operationType == UpdateOperationType::Uninstall )
                continue;

            // NOTE: opkg just can not download packages with lower version
            // TODO: fix/patch opkg
            if( !step.packageSummary.version.empty() ) {
                auto it = installedPackageVersions.find( step.packageSummary.name );
                if( it != installedPackageVersions.end() ) {
                    if( Common::BuildVersion( step.packageSummary.version ) < Common::BuildVersion( it->second ) ) {
                        Log::warn( cc::warn( "Skipping downloading of " ) + cc::note( step.packageSummary.name ) +
                                   " package" );
                        LOGGER_DEBUG_RAW( cc::debug( "Current version: " ) + cc::note( it->second ) +
                                          cc::debug( ", update version: " ) + cc::note( step.packageSummary.version ) );
                        continue;
                    }
                }
            }

            packages.push_back( step.packageSummary.name );
        }
    }

    // Download extension packages
    for( auto& package : packages ) {
        Log::info( cc::info( "Downloading " ) + cc::note( package ) + cc::info( " package" ) );
        for( int i = 0; i < RetriesCount + 1; ++i ) {
            if( i != 0 ) {
                Log::warn( cc::warn( "Retry #" ) + cc::num10( i ) + cc::warn( " in " ) +
                           cc::num10( RetriesDelay.count() ) + cc::warn( " seconds" ) );
                std::this_thread::sleep_for( RetriesDelay );
            }
            std::tie( output, result ) = m_opkgManager.createPackagesManager()->downloadPackage(
                package, Resources::CacheDirectories::Packages, false );
            Log::debug( output );

            if( result == 0 )
                break;
        }

        if( result != 0 ) {
            Log::error( cc::error( "Failed to download package" ) );
            return false;
        }
    }
    return true;
}

void Service::cleanCache() {
    if( !filesystem::deleteFolder( Resources::CacheDirectories::Update ) ) {
        Log::error( cc::error( "Failed to delete cache directory" ) );
    }
}

void Service::notifyPluginsAvailable( const std::vector< std::string >& pluginNames ) {
    if( pluginNames.empty() ) {
        return;
    }
    auto plugins = JSON_EMPTY_ARR;
    for( auto& pluginName : pluginNames ) {
        plugins.push_back( pluginName );
    }

    auto params = JSON_EMPTY_OBJ;
    params[ "plugins" ] = plugins;
    sendRequest( GroupsNames::EXTENSIONS_MANAGER, PLUGINS_AVAILABLE, params );
}

void Service::notifyPluginsUnavailable( const std::vector< std::string >& pluginNames ) {
    if( pluginNames.empty() ) {
        return;
    }
    auto plugins = JSON_EMPTY_ARR;
    for( auto& pluginName : pluginNames ) {
        plugins.push_back( pluginName );
    }

    auto params = JSON_EMPTY_OBJ;
    params[ "plugins" ] = plugins;
    sendRequest( GroupsNames::EXTENSIONS_MANAGER, PLUGINS_UNAVAILABLE, params );

    // TODO: wait until all plugins will be teared down correctly
}

void Service::notifyPluginsUnavailable( const UpdatePlan& updatePlan ) {
    std::vector< std::string > upgradePlugins;
    std::vector< std::string > removePlugins;

    for( auto& step : updatePlan.steps ) {
        if( isPluginPackage( step.packageSummary.name ) ) {
            auto pluginName = step.packageSummary.name.substr( strlen( Resources::PackageNames::PluginPrefix ) );
            getDBClient().update( Resources::DBTableNames::UnavailablePlugins,
                                  JSON_EMPTY_OBJ,
                                  JSON( db::ID_FIELD_NAME << pluginName ),
                                  true,
                                  false );

            if( step.operationType == UpdateOperationType::Upgrade ) {
                upgradePlugins.emplace_back( std::move( pluginName ) );
            }
            else if( step.operationType == UpdateOperationType::Uninstall ) {
                removePlugins.emplace_back( std::move( pluginName ) );
            }
        }
    }
    getDBClient().update( Resources::DBTableNames::UnavailablePlugins,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << "UM.initialized" ),
                          true,
                          false );

    notifyPluginsUnavailable( upgradePlugins );
    uninstallPlugins( removePlugins );
}

void Service::uninstallPlugins( const std::vector< std::string >& pluginNames ) {
    if( pluginNames.empty() ) {
        return;
    }
    auto plugins = JSON_EMPTY_ARR;
    for( auto& pluginName : pluginNames ) {
        plugins.push_back( pluginName );
    }

    auto params = JSON_EMPTY_OBJ;
    params[ "plugins" ] = plugins;
    sendRequest( GroupsNames::EXTENSIONS_MANAGER, PLUGINS_UNINSTALL, params );
}

void Service::watchForUpdate() {
    using namespace HubPlatform::HardwareManager;
    // TODO: send different broadcast for different requests

    std::this_thread::sleep_for( UpdateProgressCheckPeriod );

    std::ifstream journal( m_journalFileName );
    if( !journal.is_open() ) {
        getDBClient().update( Resources::DBTableNames::UnavailablePlugins,
                              JSON_EMPTY_OBJ,
                              JSON( db::ID_FIELD_NAME << "UM.initialized" ),
                              true,
                              false );
        return;
    }

    int scheduledSteps = 0;
    int completedSteps = 0;
    bool haveFailedSteps = false;
    bool haveUpdateFirmwareStep = false;

    std::vector< std::string > unavailablePluginNames;

    std::string line;
    while( getline( journal, line ) ) {
        scheduledSteps++;

        std::istringstream iss( line );
        std::string command, packageName, packageVersion, status;
        if( !( iss >> command >> packageName >> packageVersion >> status ) ) {
            Log::warn( cc::warn( "Can not read entry: " ) + line );
            haveFailedSteps = true;
            break;
        }

        if( !iss.eof() ) {
            Log::warn( cc::warn( "Extra characters in entry: " ) + line );
            haveFailedSteps = true;
            break;
        }

        if( packageName == Resources::PackageNames::Firmware ) {
            haveUpdateFirmwareStep = true;
        }

        if( isPluginPackage( packageName ) ) {
            std::string pluginName = packageName.substr( strlen( Resources::PackageNames::PluginPrefix ) );
            bool temporary = ( command == Resources::Commands::Install ) || ( command == Resources::Commands::Upgrade );
            getDBClient().update( Resources::DBTableNames::UnavailablePlugins,
                                  JSON_EMPTY_OBJ,
                                  JSON( db::ID_FIELD_NAME << pluginName << "temporary" << temporary ),
                                  true,
                                  false );
            unavailablePluginNames.push_back( std::move( pluginName ) );
        }

        if( status == Resources::Statuses::Failed ) {
            Log::warn( cc::warn( "Status is failed in entry: " ) + line );
            haveFailedSteps = true;
            // NOTE: Need to gather all unavailable plugin names, so continue iterating
        }

        if( status == Resources::Statuses::Completed ) {
            completedSteps++;
            if( completedSteps > m_reportedSteps ) {
                // TODO: broadcast step to EM
            }
        }
    }
    journal.close();

    getDBClient().update( Resources::DBTableNames::UnavailablePlugins,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << "UM.initialized" ),
                          true,
                          false );
    notifyPluginsUnavailable( unavailablePluginNames );

    if( scheduledSteps == 0 ) {
        // NOTE: no update in progress, nothing to do
        return;
    }

    if( completedSteps == scheduledSteps ) {
        Log::info( "Detected update process successful" );
        handleUpdateOperationSuccessful( m_operationId );
        m_opkgManager.createFeedsUpdater()->resetCurrent();
        reset();
        cleanJournal();
        cleanCache();
        cleanUnavailablePlugins();
        return;
    }

    if( !haveFailedSteps ) {
        if( m_reportedSteps != completedSteps ) {
            int updateProgress = PrepareProcessPercents + completedSteps * 80 / scheduledSteps;
            Log::info( cc::info( "Detected update process progress: " ) + cc::num10( updateProgress ) +
                       cc::info( "%" ) );
            sendProgressStatus( m_operationId, "process", updateProgress );
            handleUpdateOperationStart();
            m_reportedSteps = completedSteps;
        }
        else {
            auto result = std::get< 1 >( System::getCommandOutput( "pidof updatecmd" ) );
            if( result != 0 ) {
                ++m_noUpdateChecks;
            }
            if( m_noUpdateChecks > StallChecks ) {
                Log::error( cc::error( "Detected update process stalled" ) );
                handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
                cleanJournal();
                cleanCache();
                cleanUnavailablePlugins();
                return;
            }
        }
        return;
    }

    if( haveFailedSteps ) {
        Log::error( cc::error( "Detected update process failed" ) );
        handleUpdateOperationFailed( m_operationId, Errors::UPDATE_OPERATIONS_ERROR );
        cleanJournal();
        cleanCache();
        cleanUnavailablePlugins();
        return;
    }
}

void Service::sendProgressStatus( const std::string& operationId, std::string status, int percentage ) {
    JsonObject data;
    json_add_null( data, "error" );
    json_add_str( data, "operationId", operationId );
    json_add_str( data, "serial", getSerial() );

    json_add_int( data, "completed", percentage );
    json_add_str( data, "status", status );

    auto& dbClient = getDBClient();
    auto cData = data;
    auto currentTimestamp = std::chrono::system_clock::now().time_since_epoch();
    cData[ "timestamp" ] = currentTimestamp.count();
    if( status == Resources::PendingUpdateFields::Started )
        dbClient.insert( HaUpdateManagerPendingUpdate, cData );
    else if( status == Resources::PendingUpdateFields::Process )
        dbClient.update( HaUpdateManagerPendingUpdate,
                         JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::PendingUpdate ),
                         JSON( "$set" << cData ) );
    else if( status == Resources::PendingUpdateFields::Finished )
        dbClient.remove( HaUpdateManagerPendingUpdate,
                         JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::PendingUpdate ) );

    Services::Message message = { GroupsNames::UPDATE_MANAGER_BROADCASTS, EVENT, UPDATING_SOFTWARE, data };
    send( message );
}

void Service::sendProgressError( const std::string& operationId,
                                 Errors::Error error,
                                 const std::string& errorMessage ) {
    JsonObject data;
    setError( data, error, errorMessage );
    json_add_str( data, "operationId", operationId );
    json_add_str( data, "serial", getSerial() );

    json_add_int( data, "completed", 100 );
    json_add_str( data, "status", "aborted" );

    Services::Message message = { GroupsNames::UPDATE_MANAGER_BROADCASTS, EVENT, UPDATING_SOFTWARE, data };
    send( message );
}

void Service::reset() {
    m_operationId.clear();
    m_reportedSteps = 0;
    m_noUpdateChecks = 0;
}

void Service::cleanJournal() {
    std::ofstream ofs( m_journalFileName, std::ios_base::trunc );
    ofs.flush();
    ofs.close();

    cleanUnexpectedFolder();
}

void Service::cleanUnavailablePlugins() {
    auto query = JSON( db::ID_FIELD_NAME << JSON( "$ne"
                                                  << "UM.initialized" ) );
    auto unavailablePlugins = getDBClient().findAll( Resources::DBTableNames::UnavailablePlugins, query, nullptr );

    std::vector< std::string > pluginNames;
    for( auto& unavailablePlugin : unavailablePlugins ) {
        if( json_get_bool( unavailablePlugin, "temporary" ) ) {
            pluginNames.emplace_back( json_get_str( unavailablePlugin, db::ID_FIELD_NAME ) );
        }
    }

    getDBClient().remove( Resources::DBTableNames::UnavailablePlugins, query );
    notifyPluginsAvailable( pluginNames );
}

const std::string& Service::getSerial() {
    if( m_serial.empty() ) {
        m_serial = HubInfoHelper::getSerial();
    }
    return m_serial;
}

void Service::cleanUnexpectedFolder() {
    if( !m_journalFileName.empty() && ( m_journalFileName != "/" ) &&
        filesystem::isDirectoryExist( m_journalFileName ) ) {
        Log::warn( cc::p( m_journalFileName ) + cc::warn( " is folder but it should be file." ) );
        filesystem::deleteFolder( m_journalFileName );
    }
}

Opkg::IFeedsUpdater::Feeds Service::selectFeedType( const std::string& source, int snapshot ) {
    Log::info( cc::info( "Select feeds type for source, snapshot: " ) + source + cc::info( ", " ) +
               cc::num10( snapshot ) );

    // Select feeds to update here:
    // Step 1: check custom feeds
    bool areCustomFeedsAvailable = false;
    bool areCommonFeedsAvailable = false;
    {
        Log::info( cc::info( "Check custom feeds" ) );

        auto feedsType = Opkg::IFeedsUpdater::Feeds::Custom;
        OpkgSession checkFeeds( m_opkgManager, m_opkgManager.createFeedsUpdater( feedsType ) );
        if( !checkFeeds.updateFeeds( source, snapshot ) ) {
            Log::error( cc::error( "Can not update custom feeds" ) );
            return {};
        }
        areCustomFeedsAvailable = checkFeeds.checkAvailablePackages();
    }

    // Step 2: check common feeds
    {
        Log::info( cc::info( "Check common feeds" ) );

        auto feedsType = Opkg::IFeedsUpdater::Feeds::Common;
        OpkgSession checkFeeds( m_opkgManager, m_opkgManager.createFeedsUpdater( feedsType ) );
        if( !checkFeeds.updateFeeds( source, snapshot ) ) {
            Log::error( cc::error( "Can not update feeds" ) );
            return {};
        }
        areCommonFeedsAvailable = checkFeeds.checkAvailablePackages();
    }

    // Step 3: select common feeds or full feeds according to the previous check
    auto sessionFeedsType = Opkg::IFeedsUpdater::Feeds::Common;
    if( areCustomFeedsAvailable ) {
        LOGGER_DEBUG( cc::debug( "After feeds check, update all feeds: common and custom" ) );
        sessionFeedsType = Opkg::IFeedsUpdater::Feeds::All;
    }
    else {
        LOGGER_DEBUG( cc::debug( "After feeds check, update feeds: common only" ) );
        sessionFeedsType = Opkg::IFeedsUpdater::Feeds::Common;
    }
    return sessionFeedsType;
}

}  // namespace UpdateManager
}  // namespace HubPlatform
