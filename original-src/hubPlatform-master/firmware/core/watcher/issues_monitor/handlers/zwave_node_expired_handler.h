#pragma once

#include "watcher/issues_monitor/handlers/issue_handler_interface.h"
#include "watcher/types/zwave_node_status.h"

#include <chrono>
#include <memory>

namespace HubPlatform {
namespace Watcher {

class ServiceActivity;

namespace IssueHandlers {

class ZWaveNodeExpiredHandler : public IssueHandlerInterface {
public:
    class Impl {
    public:
        virtual Types::ZWaveNodeStatus getZWaveNodeStatus() = 0;
        virtual void restartService( const std::string& serviceName ) = 0;
    };

    ZWaveNodeExpiredHandler( std::unique_ptr< Impl > impl );
    void setZWaveNodeExpirationTimeout( std::chrono::milliseconds timeout );
    void run() override;

private:
    std::unique_ptr< Impl > m_impl;
    std::chrono::milliseconds m_expirationTimeout = std::chrono::seconds( 3600 );
    std::chrono::steady_clock::time_point m_lastStatusUpdate;
    Types::ZWaveNodeStatus m_nodeStatus{ Types::ZWaveNodeStatus::ZWAVE_NO_OPERATION };
};

class ZWaveNodeExpiredOpenwrtImpl : public ZWaveNodeExpiredHandler::Impl {
public:
    ZWaveNodeExpiredOpenwrtImpl( ServiceActivity& serviceActivity );
    Types::ZWaveNodeStatus getZWaveNodeStatus() override;
    void restartService( const std::string& serviceName ) override;

private:
    ServiceActivity& m_serviceActivity;
};

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
