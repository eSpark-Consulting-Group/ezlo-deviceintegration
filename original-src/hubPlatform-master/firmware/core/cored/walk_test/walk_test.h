#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/postponed_task.h"
#include "config_reader/config_reader.h"
#include "resources.h"
#include <string>
#include <chrono>
#include <atomic>

class IDbClient;

namespace HubPlatform {
namespace Services {
class Service;
}
namespace CoreD {
namespace WalkTest {

enum class State { Start, Finish, Update };
enum class CompletedValues { Started = 0, TerminationProgress = 60, Extended = 80, Finished = 100, Failed = 100 };
const int TestDurationSeconds = 15 * 60;
const int TestExtendSeconds = 5 * 60;

using WalkTestTerminationCallback = std::function< void() >;

class WalkTest {
public:
    explicit WalkTest( Services::Service& service, const ConfigReader& configReader );
    ~WalkTest();
    JsonObject protectSystemTestStart( const JsonObject& params );
    JsonObject protectSystemTestStop( const JsonObject& request );
    std::chrono::seconds getIntervalSeconds();
    std::chrono::seconds getTerminationIntervalSeconds();
    std::string getCurrentOperationId();
    JsonObject protectSystemTestExtend( const JsonObject& request );
    std::chrono::seconds getLeftTime();
    void setPeriodSeconds( std::chrono::seconds period );

    static Services::PostponedTaskPtr scheduleTerminationStatus(
        Services::Service& service,
        std::chrono::seconds interval,
        const WalkTestTerminationCallback& onWalkTestTermination );

    static std::chrono::seconds getConfigExtendPeriod( const ConfigReader& configReader );

    void sendProgressBroadcast( const std::string& status );
    void emitTerminationProgressStatus();
    void setDurationSeconds( std::chrono::seconds );
    void setExtendSeconds( std::chrono::seconds period );
    std::chrono::seconds getDuration();

private:
    bool isStarted();
    void setDbInfo( State state );
    void addDbRecord();
    IDbClient& getDBClient();
    void clearDbRecord();
    std::chrono::seconds getStartedTimeForTest( const std::string& operationId );
    void updateDbRecord();
    void extendPeriod();
    std::chrono::seconds getCurrentDuration();
    void sendProgress( const Json& payload );
    static CompletedValues completedValueByStatus( const std::string& status );
    void cleanTerminationStatusTask();

private:
    Services::Service& m_service;
    std::string m_operationId;
    std::chrono::seconds m_duration = std::chrono::seconds( TestDurationSeconds );
    std::chrono::seconds m_extendedPeriod = std::chrono::seconds( Resources::ConfigFields::ExtendedPeriodSeconds );
    std::chrono::seconds m_testExtendPeriod = std::chrono::seconds( TestExtendSeconds );
    Services::PostponedTaskPtr m_terminationStatusTask;
};

}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform
