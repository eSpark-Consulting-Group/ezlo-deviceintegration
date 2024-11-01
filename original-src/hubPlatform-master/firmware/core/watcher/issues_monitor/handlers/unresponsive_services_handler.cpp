#include "watcher/issues_monitor/handlers/unresponsive_services_handler.h"
#include "watcher/service_activity.h"
#include "watcher/utils/utils.h"

#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "base_utils/string_utils.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include <algorithm>
#include <sstream>

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

namespace {
UnresponsiveServicesHandler::ProcessInfoMap toProcessInfoMap( std::vector< Types::ProcessInfo > processInfoList ) {
    UnresponsiveServicesHandler::ProcessInfoMap map;
    for( auto& info : processInfoList ) {
        auto pid = info.pid;
        map.emplace( pid, std::move( info ) );
    }
    return map;
}
}  // namespace

UnresponsiveServicesHandler::UnresponsiveServicesHandler( std::unique_ptr< Impl > impl )
: m_impl( std::move( impl ) ) {}

void UnresponsiveServicesHandler::run() {
    try {
        auto heartbeatInfoMap = m_impl->readServiceHeartbeatInfoMap();
        auto processInfoMap = toProcessInfoMap( m_impl->readProcessInfoList() );
        removeDeadProcesses( heartbeatInfoMap, processInfoMap );
        auto stuckedServices = filterStuckedServices( heartbeatInfoMap );
        updateStuckedPidsSet( stuckedServices );
        restartStuckedServices( stuckedServices );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Exception in handler: " + CommonUtils::whatError( e ) );
    }
}

void UnresponsiveServicesHandler::removeDeadProcesses( ServiceHeartbeatInfoMap& heartbeatMap,
                                                       ProcessInfoMap& processInfoMap ) {
    for( auto it = heartbeatMap.begin(); it != heartbeatMap.end(); ) {
        if( processInfoMap.count( it->second.pid ) ) {
            ++it;
        }
        else {
            it = heartbeatMap.erase( it );
        }
    }
}

std::vector< Types::ServiceHeartbeatInfo > UnresponsiveServicesHandler::filterStuckedServices(
    ServiceHeartbeatInfoMap& heartbeatMap ) {
    std::vector< Types::ServiceHeartbeatInfo > stuckedServices;

    auto now = std::chrono::steady_clock::now();

    for( auto& element : heartbeatMap ) {
        auto& heartbeat = element.second;
        auto& serviceName = element.first;
        auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( now - heartbeat.lastUpdateTime );
        auto processTimeoutIt = m_perProcessHeartbeatTimeout.find( serviceName );
        bool isTimeout = false;
        if( processTimeoutIt != m_perProcessHeartbeatTimeout.cend() ) {
            auto& timeout = processTimeoutIt->second;
            // NOTE: Disable timeout if timeout value is 0
            isTimeout = ( timeout.count() > 0 ) && ( duration >= timeout );
        }
        else {
            // NOTE: Disable timeout if timeout value is 0
            isTimeout = ( m_defaultHeartbeatTimeout.count() > 0 ) && ( duration >= m_defaultHeartbeatTimeout );
        }

        if( isTimeout ) {
            stuckedServices.emplace_back( heartbeat );
        }
    }
    return stuckedServices;
}

void UnresponsiveServicesHandler::updateStuckedPidsSet( std::vector< Types::ServiceHeartbeatInfo >& stuckedServices ) {
    std::set< Types::Pid > unresponsiveSet;
    for( auto& heartbeat : stuckedServices ) {
        unresponsiveSet.emplace( heartbeat.pid );
    }
    m_impl->updateUnresponsiveProcessesSet( std::move( unresponsiveSet ) );
}

void UnresponsiveServicesHandler::restartStuckedServices(
    std::vector< Types::ServiceHeartbeatInfo >& stuckedServices ) {
    for( auto& heartbeat : stuckedServices ) {
        LOGGER_WARN( cc::warn( "Restarting stucked service: " ) + cc::note( heartbeat.serviceName ) +
                     cc::warn( ", pid: " ) + cc::num10( heartbeat.pid ) );
        m_impl->restartService( heartbeat.serviceName );
    }
}

void UnresponsiveServicesHandler::setDefaultHeartbeatTimeout( std::chrono::milliseconds timeout ) {
    m_defaultHeartbeatTimeout = timeout;
}

void UnresponsiveServicesHandler::setPerProcessHeartbeatTimeout(
    std::map< ServiceName, std::chrono::milliseconds > timeoutMap ) {
    m_perProcessHeartbeatTimeout = std::move( timeoutMap );
}

UnresponsiveServicesOpenwrtImpl::UnresponsiveServicesOpenwrtImpl( ServiceActivity& serviceActivity )
: m_serviceActivity( serviceActivity ) {}

UnresponsiveServicesHandler::ServiceHeartbeatInfoMap UnresponsiveServicesOpenwrtImpl::readServiceHeartbeatInfoMap() {
    return m_serviceActivity.getServiceHeartbeatData();
}

std::vector< Types::ProcessInfo > UnresponsiveServicesOpenwrtImpl::readProcessInfoList() {
    return Utils::readProcessInfoListFromProcFs();
}

void UnresponsiveServicesOpenwrtImpl::restartService( const std::string& serviceName ) {
    m_serviceActivity.restartService( serviceName );
}

void UnresponsiveServicesOpenwrtImpl::updateUnresponsiveProcessesSet( std::set< Types::Pid > set ) {
    m_serviceActivity.updateUnresponsiveProcessesSet( std::move( set ) );
}

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
