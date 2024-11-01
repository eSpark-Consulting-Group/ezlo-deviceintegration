#pragma once

#include "update_manager/common/um_update_plan.h"
#include "update_manager/um_opkg_session.h"
#include "update_manager/um_custom_plugin_downloader.h"

#include "hardware_manager/remote_interface/hm_remote_interface.h"

#include "service_interfaces/service.h"
#include "common_utils/addons/addon_summary.h"
#include "common_utils/hub_state.h"
#include "config_reader/config_reader.h"
#include "opkg_utils/opkg_manager.h"
#include "jsonrpc/error_codes.h"

#include <set>

namespace HubPlatform {
namespace UpdateManager {

class Service final : public Services::Service {
public:
    Service();

private:
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleEventFromBus( const Services::Message& message, const std::string& sender );
    void setUp() override;

private:
    JsonObject handleSoftwareInfoRequest( const JsonObject& json );
    JsonObject handleLockRequest( const JsonObject& json );
    JsonObject handleUnlockRequest( const JsonObject& json );
    void handleSoftwareUpdatePlanRequest( const JsonObject& json );
    void handleSoftwareUpdatePlanExecute( const JsonObject& json );
    void handleFirmwareLuupSwitch( const JsonObject& json );
    void handleBootloaderUpdate( const JsonObject& json );
    void handleRecoveryPartitionUpdate( const JsonObject& json );

private:
    void handleHubResetRequest( const JsonObject& json );

private:
    bool verifyInstalledPackageVersion( const Opkg::PackageSummary& packageSummary );

    bool isAddonPackage( const std::string& name );
    bool isPluginPackage( const std::string& name );
    bool isCustomPackage( const std::string& name );

    std::unique_ptr< UpdatePlan > requestUpdatePlan( UpdateOperation operation );

    void executeUpdatePlan( std::unique_ptr< UpdatePlan > updatePlan, const std::string& operationId );

    bool checkCustomPluginFile( const std::string& filePath );
    void deployCustomPlugin( const std::string& dearchivePath,
                             const std::string& componentPath,
                             const std::string& componentName );
    bool isCustomPluginOperationRunning( const std::string& pluginId );
    void setCustomPluginOperationRunning( const std::string& pluginId );
    void setCustomPluginOperationRunningNoLock( const std::string& pluginId );
    void unsetCustomPluginOperationRunning( const std::string& pluginId );

    void executeCustomPluginUpdateExecute( const UpdatePlan& updatePlan );

    bool predownload( const UpdatePlan& updatePlan, OpkgSession& opkgSession );
    void cleanCache();

    void notifyPluginsAvailable( const std::vector< std::string >& pluginNames );
    void notifyPluginsUnavailable( const std::vector< std::string >& pluginNames );
    void notifyPluginsUnavailable( const UpdatePlan& updatePlan );

    void uninstallPlugins( const std::vector< std::string >& pluginNames );

    void sendProgressStatus( const std::string& operationId, std::string status, int percentage );
    void sendProgressError( const std::string& operationId, Errors::Error error, const std::string& message );

    void reset();
    void cleanJournal();
    void cleanUnavailablePlugins();
    void watchForUpdate();

    void cleanUnexpectedFolder();

    const std::string& getSerial();
    void onCustomPluginDownloaded( const UpdateOperation& updateOperation, const std::string& operationId );
    void onCustomPluginFailedToDownload( const UpdateOperation& updateOperation,
                                         const std::string& operationId,
                                         const std::string& errorDescription );

    void handleUpdateOperationStart();
    void handleUpdateOperationFailedWithMessage( const std::string& operationId,
                                                 Errors::Error error,
                                                 const std::string& message = "" );
    void handleUpdateOperationFailed( const std::string& operationId,
                                      Errors::Error error = Errors::UPDATE_OPERATIONS_ERROR );
    void handleUpdateOperationSuccessful( const std::string& operationId );
    void handleUpdateFailed( const std::string& operationId, Errors::Error error = Errors::UPDATE_OPERATIONS_ERROR );
    void handleUpdateSuccessful();
    std::string getUpdatePlanSysupgradeVersion( const JsonObject& updatePlan );
    Opkg::IFeedsUpdater::Feeds selectFeedType( const std::string& source, int snapshot );

private:
    Opkg::OpkgManager m_opkgManager;
    ConfigReader m_configReader;
    std::unique_ptr< HardwareManager::RemoteInterface > m_hardwareManager;
    CustomPluginDownloader m_customPluginDownloader;
    std::string m_journalFileName;
    std::string m_serial;

    std::string m_customPluginStorage;
    std::string m_customPluginTemporaryStorage;

private:
    std::string m_operationId;
    int m_reportedSteps;
    int m_noUpdateChecks;
    std::mutex m_updateOperationsInProgressMtx;
    size_t m_updateOperationsInProgress;
    std::mutex m_updateCustomPluginOperationsMtx;
    std::set< std::string > m_updateCustomPluginOperations;
};

}  // namespace UpdateManager
}  // namespace HubPlatform
