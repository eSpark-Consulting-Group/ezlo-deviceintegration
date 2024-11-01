#pragma once

#include "service_interfaces/service.h"
#include "backupd/backup/backup_manager.h"
#include "backupd/backup/global_lock_manager.h"
#include "backupd/backup/vera_backup_manager.h"
#include "backupd/backup_messenger.h"
#include "config_reader/config_reader.h"
#include "base_utils/native_timer.h"
#include "cloud_storage/cloud_storage_interface.h"
#include "backupd/backup/daily_backup_schedule.h"

#include <memory>

namespace HubPlatform {
namespace BackupUpdateManager {

class Service final : public Services::Service, public Messenger {
public:
    Service();
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;

protected:
    void setUp() override;

private:
    void handleInfoCmdFromBus( const Json& command );

    void sendProgressStatus( MessageSubClasses processType, std::string status, int percentage ) override;
    void sendProgressError( MessageSubClasses processType,
                            Errors::Error error,
                            const std::string& reason = "" ) override;
    void sendBackupRequest( const std::string& serviceName,
                            const std::string& intermediateBackupDirectory,
                            const Json& config,
                            std::function< void( const JsonObject& message ) > responseCallback ) override;
    void sendVeraBackupRequest( const std::string& serviceName,
                                const std::string& intermediateBackupDirectory,
                                const std::string& action,
                                std::function< void( const JsonObject& message ) > responseCallback ) override;
    void startDailyBackup();

    const std::string& getSerial();

private:
    std::unique_ptr< Common::CloudStorageInterface::RemoteInterface > m_cloudStorageInterface;
    std::unique_ptr< GlobalLockManager > m_backupLockManager;
    std::unique_ptr< GlobalLockManager > m_restoreLockManager;
    std::unique_ptr< BackupManager > m_backupManager;
    std::unique_ptr< ScriptManager > m_scriptManager;
    std::unique_ptr< VeraBackupManager > m_veraBackupManager;
    std::unique_ptr< ConfigReader > m_configReader;
    std::unique_ptr< NativeTimer > m_dailyBackupTimer;
    std::unique_ptr< DailyBackupScheduler > m_dailyBackupScheduler;

    std::string m_serial;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
