#pragma once

#include "watcher/types/service_heartbeat_info.h"
#include "watcher/types/zwave_node_status.h"

#include "config_reader/config_reader.h"
#include "json_utils/json_utils.h"
#include "ubus_interface/ubus_impl.h"
#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "common_utils/hub_info_helper.h"
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <atomic>

extern "C" {
#include <libubox/blobmsg_json.h>
#include <libubus.h>
}

namespace HubPlatform {
namespace Watcher {

class ServiceActivity {
public:
    ServiceActivity();
    bool getServiceReportData( JsonObject& serviceReportData );
    bool addServiceInfo( const JsonObject& serviceInfo );
    std::map< std::string, Types::ServiceHeartbeatInfo > getServiceHeartbeatData();
    void updateUnresponsiveProcessesSet( std::set< Types::Pid > unresponsiveProcesses );
    Types::ZWaveNodeStatus getZWaveNodeStatus();
    void restartService( const std::string& serviceName );

private:
    void setServiceRunningStatus( const JsonObject& ubusResponseJson );
    Types::ZWaveNodeStatus zwaveChipHealthStatus( const JsonObject& zwaveOptions ) const;
    void rawServiceDataConsumer();
    void setDefaults();
    size_t getServicePid( const std::string& serviceFileName );
    std::string getServiceFileName( const std::string& serviceName );

private:
    std::mutex m_serviceDataMutex;
    std::map< std::string, JsonObject > m_serviceData;
    std::map< std::string, Types::ServiceHeartbeatInfo > m_serviceHeartbeatData;
    std::set< Types::Pid > m_unresponsiveProcesses;
    std::atomic< Types::ZWaveNodeStatus > m_lastZWaveNodeStatus{ Types::ZWaveNodeStatus::ZWAVE_NO_OPERATION };

    std::unique_ptr< Ubus::UBusInterface > m_uBusObject;
    // service restart guard
    std::mutex m_serviceRestartMutex;
    std::unordered_set< std::string > m_customReporterServices = { "ha-webrtcd", "ha-mqttwebrtcd", "mediamtx",
                                                                   "ha-oracode", "mosquitto",      "system_monitor" };
};

}  // namespace Watcher
}  // namespace HubPlatform
