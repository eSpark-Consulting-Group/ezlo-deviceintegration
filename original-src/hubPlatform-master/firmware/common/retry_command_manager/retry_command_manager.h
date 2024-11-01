#pragma once

#include "command.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_utils.h"
#include "service_interfaces/postponed_task.h"
#include "service_interfaces/service.h"

#include <queue>

namespace HubPlatform {
namespace Services {
class Service;
}

class RetryCommandManager {
public:
    RetryCommandManager( const std::string& commandClassId,
                         std::chrono::seconds minIntervalSeconds,
                         std::chrono::seconds maxIntervalSeconds,
                         std::chrono::seconds persistentStoringIntervalSeconds,
                         Services::Service& service,
                         bool testMode = false );

    ~RetryCommandManager();

    void sendCommand( CommandPtr command );
    std::vector< Json > loadCommandsDataFromStorage();
    static std::chrono::seconds calcRepeatInterval( int retryIndex );
    void checkResult();

private:
    void addToPersistentStorageIfNeeded( const std::string& id );
    void removeFromPersistentStorage( const std::string& id );
    void addOrUpdateDBRecord( const std::string& id, const Json& data, int retriesNumber );
    void handleFailedState();
    void checkAndClearStorageIfNeeded();
    void processCommands();
    void increaseRetryInterval();

private:
    std::mutex m_queueMutex;
    std::queue< std::shared_ptr< Command > > m_commandQueue;
    std::string m_commandClassId;
    Services::Service& m_service;
    CommandPtr m_command = nullptr;
    std::chrono::seconds m_minIntervalSeconds = std::chrono::seconds( 15 );
    std::chrono::seconds m_maxIntervalSeconds = std::chrono::minutes( 15 );
    std::chrono::seconds m_persistentStoringIntervalSeconds = std::chrono::minutes( 10 );
    Services::PostponedTaskPtr m_processTask;
    time_t m_latestStorageTimestamp = 0;
    bool m_testMode;
};
}  // namespace HubPlatform
