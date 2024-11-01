#pragma once

#include "backupd/backup/iglobal_lock_manager.h"
#include "service_interfaces/service.h"

#include <vector>
#include <set>
#include <mutex>

namespace HubPlatform {
namespace BackupUpdateManager {

class GlobalLockManager : public IGlobalLockManager {
public:
    using ResultCallback = std::function< void( const std::string& error ) >;
    using ServiceCallback = std::function< void( const std::string& service ) >;
    using Callback = std::function< void() >;

    explicit GlobalLockManager( Services::Service& service, std::string reason );
    ~GlobalLockManager();

    bool start( ResultCallback lockedCallback,
                Callback unlockedCallback,
                ServiceCallback serviceRestartedWhileLockedCallback ) override;

    bool lock() override;
    bool unlock() override;
    bool assumeUnlocked() override;

    // Should be called when detecting core or addon service start
    void onServiceStarted( const std::string& name );

    bool isLocked() const override;
    CommonUtils::HubState getHubState() const override;

private:
    void handleLockResponse( const std::string& service, bool isAddon, int operationId, const JsonObject& response );
    void handleUnlockResponse( const std::string& service, bool isAddon, int operationId, const JsonObject& response );

    // Does not lock, requires a lock
    void requestUnlockCoreServices();
    void requestUnlockService( const std::string& name, bool isAddon );

    // Does not lock, does not require a lock
    void startRecovery();
    bool hasOtherDBLock() const;
    std::string getDBLockStatus() const;
    void ensureDBLockStatus( const std::string& status );
    std::vector< std::string > loadDBAddonServices() const;
    void saveDBAddonServices( const std::vector< std::string >& addonServices );
    void removeDBLock();

private:
    Services::Service& m_service;
    const std::string m_reason;
    const std::chrono::seconds m_lockRequestTimeout;
    const std::chrono::seconds m_unlockRequestTimeout;

    static std::vector< std::string > m_coreServices;
    static std::set< std::string > m_unsupportedAddonServices;
    static int m_operationIdCounter;

    ResultCallback m_lockedCallback;
    Callback m_unlockedCallback;
    ServiceCallback m_serviceRestartedWhileLockedCallback;

    mutable std::mutex m_mutex;
    CommonUtils::HubState m_hubState;
    std::vector< std::string > m_addonServices;
    std::set< std::string > m_lockedCoreServices;
    std::set< std::string > m_lockedAddonServices;
    std::set< std::string > m_failedToUnlockServices;

    bool m_locked;
    int m_pendingLockId;
    int m_pendingUnlockId;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
