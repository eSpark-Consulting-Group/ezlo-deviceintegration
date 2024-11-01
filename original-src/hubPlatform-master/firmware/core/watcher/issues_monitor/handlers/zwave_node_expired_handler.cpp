#include "watcher/issues_monitor/handlers/zwave_node_expired_handler.h"
#include "watcher/service_activity.h"
#include "watcher/utils/utils.h"
#include "watcher/resources/watcher_resources.h"

#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

ZWaveNodeExpiredHandler::ZWaveNodeExpiredHandler( std::unique_ptr< Impl > impl )
: m_impl( std::move( impl ) ) {
    m_lastStatusUpdate = std::chrono::steady_clock::now();
}

void ZWaveNodeExpiredHandler::run() {
    try {
        auto status = m_impl->getZWaveNodeStatus();
        auto now = std::chrono::steady_clock::now();
        if( status != m_nodeStatus ) {
            m_nodeStatus = status;
            m_lastStatusUpdate = now;
        }

        if( now - m_lastStatusUpdate >= m_expirationTimeout && status == Types::ZWaveNodeStatus::ZWAVE_NODE_EXPIRED ) {
            LOGGER_WARN( cc::warn( "ZWave node expired timeout! Restarting zwave service" ) );
            m_impl->restartService( Resources::ServiceNames::ZWave );
            m_nodeStatus = Types::ZWaveNodeStatus::ZWAVE_NO_OPERATION;
            m_lastStatusUpdate = now;
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Exception in handler: " + CommonUtils::whatError( e ) );
    }
}

void ZWaveNodeExpiredHandler::setZWaveNodeExpirationTimeout( std::chrono::milliseconds timeout ) {
    m_expirationTimeout = timeout;
}

ZWaveNodeExpiredOpenwrtImpl::ZWaveNodeExpiredOpenwrtImpl( ServiceActivity& serviceActivity )
: m_serviceActivity( serviceActivity ) {}

Types::ZWaveNodeStatus ZWaveNodeExpiredOpenwrtImpl::getZWaveNodeStatus() {
    return m_serviceActivity.getZWaveNodeStatus();
}

void ZWaveNodeExpiredOpenwrtImpl::restartService( const std::string& serviceName ) {
    m_serviceActivity.restartService( serviceName );
}

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
