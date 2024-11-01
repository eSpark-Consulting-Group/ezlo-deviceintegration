#pragma once

#include "watcher/issues_monitor/handlers/issue_handler_interface.h"
#include "watcher/types/service_heartbeat_info.h"
#include "watcher/types/process_info.h"

#include <vector>
#include <chrono>
#include <memory>
#include <map>
#include <set>
#include <functional>

namespace HubPlatform {
namespace Watcher {

class ServiceActivity;

namespace IssueHandlers {

class UnresponsiveServicesHandler : public IssueHandlerInterface {
public:
    using ServiceName = std::string;
    using ProcessInfoMap = std::map< Types::Pid, Types::ProcessInfo >;
    using ServiceHeartbeatInfoMap = std::map< ServiceName, Types::ServiceHeartbeatInfo >;

    class Impl {
    public:
        virtual ServiceHeartbeatInfoMap readServiceHeartbeatInfoMap() = 0;
        virtual std::vector< Types::ProcessInfo > readProcessInfoList() = 0;
        virtual void restartService( const std::string& serviceName ) = 0;
        virtual void updateUnresponsiveProcessesSet( std::set< Types::Pid > set ) = 0;
    };

    UnresponsiveServicesHandler( std::unique_ptr< Impl > impl );

    void setDefaultHeartbeatTimeout( std::chrono::milliseconds timeout );
    void setPerProcessHeartbeatTimeout( std::map< ServiceName, std::chrono::milliseconds > processTimeoutMap );

    void run() override;

private:
    void removeDeadProcesses( ServiceHeartbeatInfoMap& heartbeatMap, ProcessInfoMap& processInfoMap );
    std::vector< Types::ServiceHeartbeatInfo > filterStuckedServices( ServiceHeartbeatInfoMap& heartbeatMap );
    void updateStuckedPidsSet( std::vector< Types::ServiceHeartbeatInfo >& stuckedServices );
    void restartStuckedServices( std::vector< Types::ServiceHeartbeatInfo >& stuckedServices );

private:
    std::unique_ptr< Impl > m_impl;
    std::chrono::milliseconds m_defaultHeartbeatTimeout = std::chrono::minutes( 10 );
    std::map< ServiceName, std::chrono::milliseconds > m_perProcessHeartbeatTimeout;
};

class UnresponsiveServicesOpenwrtImpl : public UnresponsiveServicesHandler::Impl {
public:
    UnresponsiveServicesOpenwrtImpl( ServiceActivity& serviceActivity );
    UnresponsiveServicesHandler::ServiceHeartbeatInfoMap readServiceHeartbeatInfoMap() override;
    std::vector< Types::ProcessInfo > readProcessInfoList() override;
    void restartService( const std::string& serviceName ) override;
    void updateUnresponsiveProcessesSet( std::set< Types::Pid > set ) override;

private:
    ServiceActivity& m_serviceActivity;
};

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
