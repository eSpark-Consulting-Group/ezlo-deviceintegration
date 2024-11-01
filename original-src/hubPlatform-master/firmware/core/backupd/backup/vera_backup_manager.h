#pragma once

#include "ivera_backup_manager.h"
#include "backup_script_manager.h"
#include "backupd/backup_messenger.h"

#include <chrono>
#include <set>
#include <functional>
#include <mutex>
#include <list>

namespace HubPlatform {

namespace UT {
class BackupdManagerTestSuite;
}  // namespace UT

namespace BackupUpdateManager {

class VeraBackupManager : public IVeraBackupManager {
    friend class UT::BackupdManagerTestSuite;

    struct ActionHandler {
        std::string actionName;
        std::string error;
        std::mutex mutex;
        std::list< std::string > daemonsInProgress;
    };

public:
    using OnFinishedCallback = std::function< void( std::string ) >;
    VeraBackupManager( Messenger& messenger, ScriptManagerBase& scriptManager );
    void setUp( OnFinishedCallback onFinished ) override;
    void start( std::string backupDirectory ) override;

private:
    void migrateDevices();
    void migrateRooms();
    void migrateScenes();
    bool startDaemons( const std::vector< std::string > daemons );
    void startAction( const std::string& actionName );

    void sendBackupAction( const std::string daemon, std::shared_ptr< ActionHandler > actionHandler );
    void handleActionResponse( const std::string& service,
                               std::shared_ptr< ActionHandler > handler,
                               const JsonObject& response );

private:
    OnFinishedCallback m_onFinished;
    Messenger& m_messenger;
    ScriptManagerBase& m_scriptManager;
    const std::vector< std::string > m_actionList;
    std::chrono::milliseconds m_daemonsStartupDelay = std::chrono::seconds{ 5 };

    std::string m_backupDirectory;
    std::set< std::string > m_runningDaemons;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
