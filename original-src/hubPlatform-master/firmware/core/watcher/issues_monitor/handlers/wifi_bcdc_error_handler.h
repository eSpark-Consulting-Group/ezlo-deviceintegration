#pragma once

#include "watcher/issues_monitor/handlers/issue_handler_interface.h"
#include "watcher/system_log/system_log_interface.h"

#include <vector>
#include <memory>
#include <mutex>

class IDbClient;

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

class WifiBcdcErrorHandler : public IssueHandlerInterface {
public:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual void runRecoveryAction() = 0;
    };

    WifiBcdcErrorHandler( IDbClient& dbClient, SystemLogInterface& systemLog );
    WifiBcdcErrorHandler( IDbClient& dbClient, SystemLogInterface& systemLog, std::unique_ptr< Impl > impl );
    ~WifiBcdcErrorHandler();

    void run() override;

private:
    void handleLogRecordEvent( const std::string& logRecord );

private:
    IDbClient& m_dbClient;
    std::unique_ptr< Impl > m_impl;
    SystemLogInterface& m_systemLog;
    std::mutex m_mutex;
    std::vector< std::string > m_logRecords;
};

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
