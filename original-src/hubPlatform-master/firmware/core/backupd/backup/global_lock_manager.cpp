#include "backupd/backup/global_lock_manager.h"
#include "update_manager/remote_interface/um_remote_interface.h"
#include "system_utils/environment.h"

#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace BackupUpdateManager {

namespace {
auto DefaultLockRequestTimeout = std::chrono::seconds( 30 );
auto DefaultUnlockRequestTimeout = std::chrono::seconds( 30 );
auto RestartWaitTime = std::chrono::seconds( 10 );
auto GlobalLocksDBName = "temp.db.firmware.backupd.locks";

auto LockedStatus = "locked";
auto UnlockingStatus = "unlocking";
auto RecoveredStatus = "recovered";

}  // namespace

// NOTE: All core services should be listed here
std::vector< std::string > GlobalLockManager::m_coreServices = {
    "cored", "network", "ui_service", "update_manager", "extensions_manager", "external_controllers", "hardware_manager"
};

std::set< std::string > GlobalLockManager::m_unsupportedAddonServices = { "addon.luup", "addon.bluetooth" };

int GlobalLockManager::m_operationIdCounter = 0;

GlobalLockManager::GlobalLockManager( Services::Service& service, std::string reason )
: m_service( service )
, m_reason( std::move( reason ) )
, m_lockRequestTimeout( DefaultLockRequestTimeout )
, m_unlockRequestTimeout( DefaultUnlockRequestTimeout )
, m_lockedCallback( []( const std::string& ) {} )
, m_unlockedCallback( []() {} )
, m_serviceRestartedWhileLockedCallback( []( const std::string& ) {} )
, m_locked( false )
, m_pendingLockId( 0 )
, m_pendingUnlockId( 0 ) {}

GlobalLockManager::~GlobalLockManager() {
    // Nothing meaningful can be done here, delegate clean up to following start logic
}

bool GlobalLockManager::start( ResultCallback lockedCallback,
                               Callback unlockedCallback,
                               ServiceCallback serviceRestartedWhileLockedCallback ) {
    std::unique_lock< std::mutex > guard( m_mutex );
    m_lockedCallback = std::move( lockedCallback );
    m_unlockedCallback = std::move( unlockedCallback );
    m_serviceRestartedWhileLockedCallback = std::move( serviceRestartedWhileLockedCallback );

    // If we have restarted while locking or being locked, unlock firmware
    auto status = getDBLockStatus();
    if( status == LockedStatus ) {
        LOGGER_ERROR_RAW( cc::error( "Unexpected service restart during locked state" ) );
        m_locked = true;
        m_addonServices = loadDBAddonServices();
        m_lockedCoreServices.insert( m_coreServices.begin(), m_coreServices.end() );
        m_lockedAddonServices.insert( m_addonServices.begin(), m_addonServices.end() );
        guard.unlock();
        // NOTE: Give some time for all services to start, otherwise unlock requests will timeout
        std::this_thread::sleep_for( RestartWaitTime );
        unlock();
        return false;
    }
    else if( status == UnlockingStatus ) {
        LOGGER_ERROR_RAW( cc::error( "Unexpected service restart during unlocking services" ) );
        startRecovery();
        return false;
    }
    else if( status == RecoveredStatus ) {
        LOGGER_INFO_RAW( cc::info( "Recovered from failed unlock" ) );
        removeDBLock();
        guard.unlock();
        // NOTE: Give some time for all services to start
        std::this_thread::sleep_for( RestartWaitTime );
        m_unlockedCallback();
    }
    return true;
}

bool GlobalLockManager::lock() {
    LOGGER_DEBUG_RAW( cc::debug( "Trying to perform firmware lock for " ) + cc::note( m_reason ) );
    std::unique_lock< std::mutex > guard( m_mutex );
    if( m_pendingLockId || m_locked || m_pendingUnlockId ) {
        guard.unlock();
        LOGGER_WARN_RAW( cc::warn( "Another lock operation in progress or already locked" ) );
        return false;
    }

    ensureDBLockStatus( LockedStatus );

    if( hasOtherDBLock() ) {
        removeDBLock();
        guard.unlock();
        LOGGER_WARN_RAW( cc::warn( "Already locked for another reason" ) );
        return false;
    }

    m_addonServices.clear();
    m_lockedCoreServices.clear();
    m_lockedAddonServices.clear();
    m_hubState = {};
    m_pendingUnlockId = 0;
    m_pendingLockId = ++m_operationIdCounter;
    for( const auto& service : m_coreServices ) {
        LOGGER_DEBUG_RAW( cc::debug( "Sending lock request to " ) + cc::note( service ) + cc::debug( " service" ) );
        auto message = JSON( "reason" << m_reason );
        m_service.sendAsyncRequest(
            service,
            message,
            GLOBAL_LOCK,
            std::bind(
                &GlobalLockManager::handleLockResponse, this, service, false, m_pendingLockId, std::placeholders::_1 ),
            m_lockRequestTimeout );
    }
    return true;
}

bool GlobalLockManager::unlock() {
    LOGGER_DEBUG_RAW( cc::debug( "Trying to perform firmware unlock from " ) + cc::note( m_reason ) );
    std::unique_lock< std::mutex > guard( m_mutex );
    if( !m_pendingLockId && !m_locked ) {
        LOGGER_WARN_RAW( cc::warn( "No lock operations in progress or not locked yet" ) );
        guard.unlock();
        return false;
    }

    m_failedToUnlockServices.clear();
    m_pendingLockId = 0;
    m_pendingUnlockId = ++m_operationIdCounter;
    m_hubState = {};
    ensureDBLockStatus( UnlockingStatus );

    if( m_addonServices.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "No addons to unlock, unlocking core services" ) );
        requestUnlockCoreServices();
    }
    else {
        // First unlock addons
        for( const auto& service : m_addonServices ) {
            // Wait for unlock response from all addons, even if some were not locked properly
            m_lockedAddonServices.insert( service );
            requestUnlockService( service, true );
        }
    }

    return true;
}

bool GlobalLockManager::assumeUnlocked() {
    LOGGER_DEBUG_RAW( cc::debug( "Assume firmware unlocked from " ) + cc::note( m_reason ) );
    std::unique_lock< std::mutex > guard( m_mutex );
    if( !m_pendingLockId && !m_locked ) {
        LOGGER_WARN_RAW( cc::warn( "No lock operations in progress or not locked yet" ) );
        guard.unlock();
        return false;
    }

    m_hubState = {};
    m_addonServices.clear();
    m_lockedCoreServices.clear();
    m_lockedAddonServices.clear();
    m_failedToUnlockServices.clear();

    m_locked = false;
    m_pendingLockId = 0;
    m_pendingUnlockId = 0;
    removeDBLock();
    guard.unlock();

    return true;
}

bool GlobalLockManager::isLocked() const {
    std::lock_guard< std::mutex > guard( m_mutex );
    return m_locked;
}

CommonUtils::HubState GlobalLockManager::getHubState() const {
    std::lock_guard< std::mutex > guard( m_mutex );
    return m_hubState;
}

void GlobalLockManager::handleLockResponse( const std::string& service,
                                            bool isAddon,
                                            int operationId,
                                            const JsonObject& response ) {
    std::unique_lock< std::mutex > guard( m_mutex );
    if( m_pendingLockId != operationId ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got outdated response on lock request from " ) + cc::note( service ) +
                          cc::debug( " service" ) );
        // Ignore this response, looks like we've canceled this lock already
        return;
    }

    if( hasError( response ) || !json_get_bool( response, "success", false ) ) {
        std::string message = json_get_str( response[ "error" ], "message" );
        std::string reason = json_get_str( response[ "error" ], "reason" );
        reason = message.empty() ? "Unknown reason" : message + ( reason.empty() ? "" : ": " + reason );
        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) + cc::error( " failed to lock: " ) +
                          cc::note( reason ) );
        guard.unlock();
        m_lockedCallback( "Service " + service + " failed to lock: " + reason );
        unlock();
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Service " ) + cc::note( service ) + cc::debug( " locked successfully" ) );
    if( isAddon ) {
        m_lockedAddonServices.insert( service );
    }
    else {
        m_lockedCoreServices.insert( service );
    }

    if( service == "update_manager" ) {
        LOGGER_DEBUG_RAW( cc::debug( "Getting list of addon services" ) );
        // Once update manager is locked we can get addon services list and lock them.
        UpdateManager::RemoteInterface updateManager( m_service );
        m_hubState = updateManager.requestSoftwareInfo();

        m_addonServices.clear();
        for( auto& addon : m_hubState.addons ) {
            auto addonService = "addon." + addon.id;
            if( m_unsupportedAddonServices.count( addonService ) ) {
                continue;
            }
            m_addonServices.push_back( addonService );
        }
        saveDBAddonServices( m_addonServices );

        for( auto& addonService : m_addonServices ) {
            LOGGER_DEBUG_RAW( cc::debug( "Sending lock request to " ) + cc::note( addonService ) +
                              cc::debug( " service" ) );
            auto message = JSON( "reason" << m_reason );
            m_service.sendAsyncRequest( addonService,
                                        message,
                                        GLOBAL_LOCK,
                                        std::bind( &GlobalLockManager::handleLockResponse,
                                                   this,
                                                   addonService,
                                                   true,
                                                   m_pendingLockId,
                                                   std::placeholders::_1 ),
                                        m_unlockRequestTimeout );
        }
    }

    if( ( m_lockedCoreServices.size() != m_coreServices.size() ) ||
        ( m_lockedAddonServices.size() != m_addonServices.size() ) ) {
        // Wait for other services locks
        return;
    }

    // All services are locked
    LOGGER_DEBUG_RAW( cc::debug( "All services locked successfully" ) );
    m_pendingLockId = 0;
    m_locked = true;
    guard.unlock();
    m_lockedCallback( "" );
}

void GlobalLockManager::handleUnlockResponse( const std::string& service,
                                              bool isAddon,
                                              int operationId,
                                              const JsonObject& response ) {
    std::unique_lock< std::mutex > guard( m_mutex );
    if( m_pendingUnlockId != operationId ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got outdated response on unlock request from " ) + cc::note( service ) +
                          cc::debug( " service" ) );
        // Ignore this response, looks like we've canceled this lock already
        return;
    }

    if( hasError( response ) || !json_get_bool( response, "success", false ) ) {
        std::string message = json_get_str( response[ "error" ], "message" );
        std::string reason = json_get_str( response[ "error" ], "reason" );
        reason = message.empty() ? "Unknown reason" : message + ( reason.empty() ? "" : ": " + reason );
        LOGGER_ERROR_RAW( cc::error( "Service " ) + cc::note( service ) + cc::error( " failed to unlock: " ) +
                          cc::note( reason ) );
        if( m_failedToUnlockServices.count( service ) == 0 ) {
            // Try sending unlock request again
            m_failedToUnlockServices.insert( service );
            requestUnlockService( service, isAddon );
        }
        else {
            // If second unlock attempt fails, restore firmware functionality by restarting it
            startRecovery();
        }
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Service " ) + cc::note( service ) + cc::debug( " unlocked successfully" ) );
    if( isAddon ) {
        m_lockedAddonServices.erase( service );

        if( m_lockedAddonServices.empty() ) {
            // Once all addons are unlocked we can start unlocking core services.
            LOGGER_DEBUG_RAW( cc::debug( "All addon services unlocked" ) );
            requestUnlockCoreServices();
        }
        return;
    }

    m_lockedCoreServices.erase( service );

    if( !m_lockedCoreServices.empty() ) {
        // Wait for other services unlocks
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "All services unlocked" ) );
    m_pendingUnlockId = 0;
    m_locked = false;
    removeDBLock();
    guard.unlock();
    m_unlockedCallback();
}

void GlobalLockManager::requestUnlockCoreServices() {
    for( const auto& coreService : m_coreServices ) {
        // Wait for unlock response from all core services, even if some were not locked properly
        m_lockedCoreServices.insert( coreService );
        requestUnlockService( coreService, false );
    }
}

void GlobalLockManager::requestUnlockService( const std::string& name, bool isAddon ) {
    LOGGER_DEBUG_RAW( cc::debug( "Sending unlock request to " ) + cc::note( name ) + cc::debug( " service" ) );
    auto message = JSON( "reason" << m_reason );
    m_service.sendAsyncRequest(
        name,
        message,
        GLOBAL_UNLOCK,
        std::bind(
            &GlobalLockManager::handleUnlockResponse, this, name, isAddon, m_pendingUnlockId, std::placeholders::_1 ),
        m_unlockRequestTimeout );
}

void GlobalLockManager::onServiceStarted( const std::string& name ) {
    std::unique_lock< std::mutex > guard( m_mutex );
    bool serviceLocked = ( m_lockedCoreServices.find( name ) != m_lockedCoreServices.end() ) ||
                         ( m_lockedAddonServices.find( name ) != m_lockedAddonServices.end() );
    // Do nothing for services that are not locked or if we are unlocking
    if( serviceLocked && !m_pendingUnlockId ) {
        if( m_pendingLockId ) {
            guard.unlock();
            m_lockedCallback( "Service " + name + " restarted" );
        }
        else {
            guard.unlock();
            m_serviceRestartedWhileLockedCallback( name );
        }
        unlock();
    }
}

void GlobalLockManager::startRecovery() {
    LOGGER_INFO_RAW( cc::error( "Restarting firmware..." ) );
    ensureDBLockStatus( RecoveredStatus );
    system( ( HubPlatform::Environment::getBinariesFolder() + "/updatecmd --daemon --restart_firmware" ).c_str() );
    std::this_thread::sleep_for( std::chrono::seconds( 60 ) );
}

bool GlobalLockManager::hasOtherDBLock() const {
    return m_service.getDBClient().has( GlobalLocksDBName, JSON( "_id" << JSON( "$ne" << m_reason ) ) );
}

std::string GlobalLockManager::getDBLockStatus() const {
    auto lock = m_service.getDBClient().findOne( GlobalLocksDBName, JSON( "_id" << m_reason ), nullptr );
    if( lock.empty() ) {
        return "";
    }
    return json_get_str( lock, "status" );
}

void GlobalLockManager::ensureDBLockStatus( const std::string& status ) {
    m_service.getDBClient().update( GlobalLocksDBName, {}, JSON( "_id" << m_reason << "status" << status ), true );
}

std::vector< std::string > GlobalLockManager::loadDBAddonServices() const {
    auto lock = m_service.getDBClient().findOne( GlobalLocksDBName, JSON( "_id" << m_reason ), nullptr );
    if( lock.empty() ) {
        return {};
    }
    return json_get_vec< std::string >( lock, "addonServices" );
}

void GlobalLockManager::saveDBAddonServices( const std::vector< std::string >& addonServices ) {
    m_service.getDBClient().update( GlobalLocksDBName,
                                    JSON( "_id" << m_reason ),
                                    JSON( "$set" << JSON( "addonServices" << addonServices ) ),
                                    true );
}

void GlobalLockManager::removeDBLock() {
    m_service.getDBClient().remove( GlobalLocksDBName, JSON( "_id" << m_reason ) );
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
