#pragma once

#include "watcher/issues_monitor/handlers/issue_handler_interface.h"

#include "service_interfaces/task_loop.h"

#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace HubPlatform {
namespace Watcher {

class IssuesMonitor {
public:
    using HandlerPtr = std::unique_ptr< IssueHandlers::IssueHandlerInterface >;
    using HandlersList = std::vector< HandlerPtr >;
    void addHandler( HandlerPtr handler, std::chrono::milliseconds checkInterval );
    ~IssuesMonitor();

    void run();
    void stop();

private:
    HandlersList m_handlers;
    HubPlatform::Services::TaskLoop m_taskLoop;
};

}  // namespace Watcher
}  // namespace HubPlatform
