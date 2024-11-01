#pragma once

#include "watcher/service_activity.h"
#include "watcher/system_activity.h"
#include "watcher/issues_monitor/issues_monitor.h"
#include "watcher/system_log/system_log.h"
#include "config_reader/config_reader.h"
#include "service_interfaces/task_loop.h"

#include "system_utils/thread_unique_safe_wrapper.h"

#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

extern "C" {
#include <libubox/blobmsg_json.h>
#include <libubus.h>
}

#define WAIT_FOR 5

class IDbClient;

namespace HubPlatform {
namespace Watcher {

void signalHandler( int signal );
void setWatcherExitSignalStatus( bool status );
bool getWatcherExitSignalStatus();
void setWatcherUnhandledSignal( bool status );
bool getWatcherUnhandledStatus();

class Service {
public:
    Service();
    ~Service();
    void uBusRun();
    bool uBusThreadRunning = false;
    void stopWatcherUloop();
    // int getWatcherConnection();
    std::thread ubusRunThread;
    int getWatcherUbusConnectionStatus();

public:
    struct WatcherInfo {
        std::atomic_size_t receivedReports{ 0 };
        std::atomic_size_t lastReceivedReport{ 0 };
        std::atomic_size_t sentReports{ 0 };
        std::atomic_size_t lastSentReport{ 0 };
    };

    WatcherInfo& getWatcherInfoRef();

private:
    void collectMemory( struct memory& meory );
    bool addServiceInfo( const std::string& serviceInfo, const std::string& serviceMessage );

    static int ubusGetInfoHandler( struct ubus_context* ctx,
                                   struct ubus_object* obj,
                                   struct ubus_request_data* req,
                                   const char* method,
                                   struct blob_attr* msg );

    static int ubusSendStatusHandler( struct ubus_context* ctx,
                                      struct ubus_object* obj,
                                      struct ubus_request_data* req,
                                      const char* method,
                                      struct blob_attr* msg );
    Json getWatcherMetrics();

private:
    const char* const serverName = "watcher";

    HubPlatform::Services::TaskLoop m_taskLoop;
    std::thread m_taskLoopThread;

    WatcherInfo m_watcherInfo;

    ConfigReader m_configReader;
    SystemActivity m_system;
    ServiceActivity m_service;
    ubus_object m_serverObject;
    ubus_method m_serverMethods[ 2 ];
    ubus_object_type m_serverObjectType;
    std::unique_ptr< Ubus::UBusInterface > m_uBusObject;
    std::unique_ptr< IDbClient > m_dbClient;

    enum { SERVER_ID, SERVICE_MSG, __MESSAGE_POLICY_MAX };
    blobmsg_policy m_messagePolicy[ __MESSAGE_POLICY_MAX ];

    std::thread m_issuesMonitorThread;
    std::unique_ptr< IssuesMonitor > m_issuesMonitor;
    SystemLog m_systemLog;

    std::chrono::steady_clock::time_point m_serviceStartupTime;
};

void stopUBusThread( std::shared_ptr< Service >& watcherObj );

}  // namespace Watcher
}  // namespace HubPlatform
