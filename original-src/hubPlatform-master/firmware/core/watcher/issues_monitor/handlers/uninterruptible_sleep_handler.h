#pragma once

#include "watcher/issues_monitor/handlers/issue_handler_interface.h"
#include "watcher/types/process_info.h"
#include "watcher/types/process_pid.h"

#include <vector>
#include <chrono>
#include <memory>
#include <map>

class IDbClient;

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

class UninterruptibleSleepHandler : public IssueHandlerInterface {
public:
    using ProcessInfoList = std::vector< Types::ProcessInfo >;

    class Impl {
    public:
        virtual ~Impl() = default;
        virtual ProcessInfoList readProcessInfoList() = 0;
        virtual void runRecoveryAction() = 0;
    };

    using ProcessInfoMap = std::map< Types::Pid, Types::ProcessInfo >;

    UninterruptibleSleepHandler( IDbClient& dbClient );
    UninterruptibleSleepHandler( IDbClient& dbClient, std::unique_ptr< Impl > impl );

    void setUninterruptibleSleepTimeout( std::chrono::milliseconds timeout );

    void run() override;

private:
    ProcessInfoMap filterUninterruptibleProcesses( const ProcessInfoList& processList );
    void updateTrackingProcesses( const ProcessInfoMap& uninterruptibleMap );
    void detectStuckedProcesses( const ProcessInfoMap& uninterruptibleMap );

private:
    IDbClient& m_dbClient;
    std::unique_ptr< Impl > m_impl;
    std::chrono::milliseconds m_uninterruptibleSleepTimeout{ 180000 };
    std::map< Types::Pid, std::chrono::steady_clock::time_point > m_trackedProcesses;
    size_t m_recoveryAttempts = 0;
};

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
