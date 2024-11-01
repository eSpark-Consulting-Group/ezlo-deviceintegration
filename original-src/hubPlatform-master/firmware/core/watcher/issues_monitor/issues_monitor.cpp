#include "watcher/issues_monitor/issues_monitor.h"

#include "common_utils/cpp14.h"

#include <functional>

namespace HubPlatform {
namespace Watcher {

IssuesMonitor::~IssuesMonitor() {
    stop();
}

void IssuesMonitor::run() {
    m_taskLoop.run();
}

void IssuesMonitor::stop() {
    m_taskLoop.stop();
}

void IssuesMonitor::addHandler( HandlerPtr handler, std::chrono::milliseconds checkInterval ) {
    m_handlers.emplace_back( std::move( handler ) );
    auto* handlerPtr = m_handlers.back().get();
    m_taskLoop.createPostponedTask( [handlerPtr]( Services::PostponedTask& ) { handlerPtr->run(); } )
        ->start( checkInterval, true /*periodic*/ );
}

}  // namespace Watcher
}  // namespace HubPlatform
