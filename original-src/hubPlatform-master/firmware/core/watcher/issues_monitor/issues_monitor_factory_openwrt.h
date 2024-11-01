#pragma once

#include "watcher/issues_monitor/issues_monitor.h"

#include <memory>

class IDbClient;

namespace HubPlatform {

class ConfigReader;

namespace Watcher {

class ServiceActivity;
class SystemLogInterface;

namespace IssuesMonitorFactoryOpenwrt {

std::unique_ptr< IssuesMonitor > createIssuesMonitor( ConfigReader& config,
                                                      ServiceActivity& service,
                                                      IDbClient& dbClient,
                                                      SystemLogInterface& systemLog );

}  // namespace IssuesMonitorFactoryOpenwrt
}  // namespace Watcher
}  // namespace HubPlatform
