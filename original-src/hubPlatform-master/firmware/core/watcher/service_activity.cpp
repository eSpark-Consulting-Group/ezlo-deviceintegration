#include "watcher/service_activity.h"

#include "watcher/resources/watcher_resources.h"

#include "json_utils/json_wrapper.h"
#include "ubus_interface/ubus_impl.h"
#include "common_utils/cpp14.h"
#include "config_reader/config_keys.h"

#include <mutex>
#include <unordered_set>
#include <utility>

namespace HubPlatform {
namespace Watcher {

ServiceActivity::ServiceActivity() {
    m_uBusObject = std::make_unique< Ubus::UBusImpl >();
    m_uBusObject->connect( nullptr );
}

bool ServiceActivity::addServiceInfo( const JsonObject& serviceInfo ) {
    JsonObject innerJson;
    json_get( serviceInfo, Watcher::Resources::ServiceActivity::service, innerJson );
    if( innerJson.empty() ) {
        LOGGER_DEBUG( cc::debug( "No service data found in Json" ) );
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    std::lock_guard< std::mutex > lock( m_serviceDataMutex );
    for( auto it = innerJson.begin(); it != innerJson.end(); ++it ) {
        const auto& serviceName = it.key();
        m_serviceData[ serviceName ] = it.value();

        auto& heartbeat = m_serviceHeartbeatData[ it.key() ];
        heartbeat.pid = json_get_int( it.value(), Watcher::Resources::ServiceActivity::pid, 0 );
        heartbeat.serviceName = serviceName;
        heartbeat.lastUpdateTime = now;

        if( serviceName == Resources::ServiceNames::ZWave ) {
            m_lastZWaveNodeStatus = zwaveChipHealthStatus( it.value() );
        }
    }

    return true;
}

std::map< std::string, Types::ServiceHeartbeatInfo > ServiceActivity::getServiceHeartbeatData() {
    std::lock_guard< std::mutex > lock( m_serviceDataMutex );
    return m_serviceHeartbeatData;
}

void ServiceActivity::updateUnresponsiveProcessesSet( std::set< Types::Pid > unresponsiveProcesses ) {
    std::lock_guard< std::mutex > lock( m_serviceDataMutex );
    m_unresponsiveProcesses = std::move( unresponsiveProcesses );
}

void ServiceActivity::setServiceRunningStatus( const JsonObject& ubusResponseJson ) {
    std::unordered_set< std::string > serviceSet;

    for( auto it = ubusResponseJson.begin(); it != ubusResponseJson.end(); ++it ) {
        std::string serviceName;
        if( json_get( it.value(),
                      { Watcher::Resources::ServiceActivity::instances,
                        Watcher::Resources::ServiceActivity::instance1,
                        Watcher::Resources::ServiceActivity::data,
                        Watcher::Resources::ServiceActivity::serviceName },
                      serviceName ) ) {
            serviceSet.insert( serviceName );
        }
    }

    for( auto& element : m_serviceData ) {
        auto& serviceName = element.first;
        auto& data = element.second;
        bool isCustomReport = json_get_bool( data, "custom_report", false );
        if( serviceSet.find( serviceName ) != serviceSet.end() || isCustomReport ||
            m_customReporterServices.find( serviceName ) != m_customReporterServices.end() ) {
            json_add_bool( data, Watcher::Resources::ServiceActivity::serviceRunningStatus, true );
        }
        else {
            json_add_bool( data, Watcher::Resources::ServiceActivity::serviceRunningStatus, false );
        }
    }
}

Types::ZWaveNodeStatus ServiceActivity::zwaveChipHealthStatus( const JsonObject& zwaveServiceInfo ) const {
    const int ALWAYS_ON_DEVICES_INTERVAL = 10800;

    // Z-Wave Service Restart When the Chip Became Unresponsive
    JsonObject zwaveNodeUpdateTimes;
    uint32_t alwaysOnDevicesInterval;

    if( !json_get(
            zwaveServiceInfo, Watcher::Resources::ServiceActivity::zwaveNodeUpdateTimes, zwaveNodeUpdateTimes ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Could not find zwave_node_update_times in zwave Json data" ) );
        return Types::ZWaveNodeStatus::ZWAVE_NODE_NIL;
    }

    if( zwaveNodeUpdateTimes.is_null() ) {
        return Types::ZWaveNodeStatus::ZWAVE_NO_OPERATION;
    }

    if( !json_get( zwaveServiceInfo,
                   Watcher::Resources::ServiceActivity::alwaysOnDevicesInterval,
                   alwaysOnDevicesInterval ) ) {
        alwaysOnDevicesInterval = static_cast< uint32_t >( ALWAYS_ON_DEVICES_INTERVAL );
        LOGGER_DEBUG_RAW( cc::debug( "Could not find always_on_devices_interval, settings to default value: " +
                                     std::to_string( alwaysOnDevicesInterval ) + "!" ) );
    }

    int64_t now = std::chrono::time_point_cast< std::chrono::seconds >( std::chrono::steady_clock::now() )
                      .time_since_epoch()
                      .count();
    for( auto it = zwaveNodeUpdateTimes.begin(); it != zwaveNodeUpdateTimes.end(); ++it ) {
        bool isWakeUp = json_get_bool( it.value(), Watcher::Resources::ServiceActivity::isWakeUp );
        int64_t lastUpdateTime = json_get_int( it.value(), Watcher::Resources::ServiceActivity::zwaveNodeLastUpdate );
        uint64_t wakeUpInterval = json_get_uint( it.value(), Watcher::Resources::ServiceActivity::wakeUpInterval );

        if( !isWakeUp || wakeUpInterval == 0 ) {
            wakeUpInterval = alwaysOnDevicesInterval;
        }
        if( ( now - lastUpdateTime ) <= wakeUpInterval ) {
            return Types::ZWaveNodeStatus::ZWAVE_NODE_NOT_EXPIRED;
        }
    }
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ ) || defined( __HUB_TYPE_G150__ )
    // There is no Z-Wave chip for these hubs, we just skipping.
    LOGGER_DEBUG_RAW( cc::warn( "Z-Wave: local configuration, Z-Wave skipped" ) );
    return Types::ZWaveNodeStatus::ZWAVE_NO_OPERATION;
#else
    return Types::ZWaveNodeStatus::ZWAVE_NODE_EXPIRED;
#endif
}

size_t ServiceActivity::getServicePid( const std::string& serviceFilename ) {
    std::string ubuServiceResponse;
    JsonObject serviceJsonInfo;
    int servicePid;
    m_uBusObject->sendCommand( Watcher::Resources::ServiceActivity::service,
                               Watcher::Resources::ServiceActivity::list,
                               "name",
                               serviceFilename,
                               ubuServiceResponse );

    parseJson( ubuServiceResponse, serviceJsonInfo );
    if( !json_get( serviceJsonInfo,
                   { serviceFilename,
                     Watcher::Resources::ServiceActivity::instances,
                     Watcher::Resources::ServiceActivity::instance1,
                     Watcher::Resources::ServiceActivity::pid },
                   servicePid ) ) {
        LOGGER_ERROR_RAW( cc::error( "Could not get service PID from uBus : " + serviceFilename ) );
        return 0;
    }
    return servicePid;
}

std::string ServiceActivity::getServiceFileName( const std::string& serviceName ) {
    std::string serviceInfo;
    std::string tmpServiceName;
    JsonObject serviceInfoJson;

    if( !m_uBusObject->sendCommand( Watcher::Resources::ServiceActivity::service,
                                    Watcher::Resources::ServiceActivity::list,
                                    "",
                                    "",
                                    serviceInfo ) ) {
        LOGGER_ERROR_RAW( cc::error( "uBus service call failed while getting service list!" ) );
        return Watcher::Resources::ServiceActivity::notAvailable;
    }

    if( !parseJson( serviceInfo, serviceInfoJson ) ) {
        LOGGER_ERROR_RAW( cc::error( "Parsing uBus call to Json object failed, data: " + serviceInfo ) );
        return Watcher::Resources::ServiceActivity::notAvailable;
    }

    for( auto it = serviceInfoJson.begin(); it != serviceInfoJson.end(); ++it ) {
        if( json_get( it.value(),
                      { Watcher::Resources::ServiceActivity::instances,
                        Watcher::Resources::ServiceActivity::instance1,
                        Watcher::Resources::ServiceActivity::data,
                        "service_name" },
                      tmpServiceName ) ) {
            if( tmpServiceName.compare( serviceName ) == 0 ) {
                return it.key();
            }
        }
    }
    return Watcher::Resources::ServiceActivity::notAvailable;
}

void ServiceActivity::restartService( const std::string& serviceName ) {
    std::lock_guard< std::mutex > lock( m_serviceRestartMutex );

    std::string serviceFileName;
    std::pair< size_t, size_t > servicePids;

    LOGGER_INFO_RAW( cc::info( "Service restarting: " + serviceName ) );
    try {
        serviceFileName = getServiceFileName( serviceName );
        if( serviceFileName == Watcher::Resources::ServiceActivity::notAvailable ) {
            LOGGER_ERROR_RAW( cc::error( "Could not get service file name while restarting service: " + serviceName ) );
            return;
        }

        servicePids.first = getServicePid( serviceFileName );
        auto restartCommandOutput = HubPlatform::System::getCommandOutput(
            std::string( "/etc/init.d/" + serviceFileName + " restart" ).c_str(), true );
        if( !restartCommandOutput.first.empty() ) {
            LOGGER_INFO_RAW( cc::debug( "Service restart command returned: " + restartCommandOutput.first +
                                        ", it should be empty." ) );
        }

        servicePids.second = getServicePid( serviceFileName );
        if( servicePids.first != servicePids.second ) {
            LOGGER_INFO_RAW( cc::info( "Service restart successful: " + serviceName ) );
        }
        else {
            LOGGER_ERROR_RAW( cc::info( "Service restart operation failed!: " + serviceName ) );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception caught while restarting service: " ) + serviceName + ", " +
                          cc::note( e.what() ) );
    }
}

bool ServiceActivity::getServiceReportData( JsonObject& serviceReportData ) {
    JsonObject serviceJson;
    JsonObject tmpJson;

    std::string ubusResponse;
    JsonObject ubusResponseJson;
    JsonObject instanceJson;
    int respwanCount = 0;
    int pid = 0;

    uint64_t serviceReportDate = 0;

    std::map< int, int > respwanCountMap;

    m_uBusObject->sendCommand(
        Watcher::Resources::ServiceActivity::service, Watcher::Resources::ServiceActivity::list, "", "", ubusResponse );
    parseJson( ubusResponse, ubusResponseJson );

    for( auto it = ubusResponseJson.begin(); it != ubusResponseJson.end(); ++it ) {
        if( json_get(
                it.value(),
                { Watcher::Resources::ServiceActivity::instances, Watcher::Resources::ServiceActivity::instance1 },
                instanceJson ) ) {
            if( json_get( instanceJson, Watcher::Resources::ServiceActivity::pid, pid ) ) {
                if( json_get( instanceJson, Watcher::Resources::ServiceActivity::respawncount, respwanCount ) ) {
                    respwanCountMap[ pid ] = respwanCount;
                }
            }
        }
    }

    std::lock_guard< std::mutex > lock( m_serviceDataMutex );
    setServiceRunningStatus( ubusResponseJson );

    std::time_t curTime = std::time( nullptr );
    for( auto& service : m_serviceData ) {
        const auto& serviceName = service.first;
        tmpJson = service.second;
        if( json_get( tmpJson, Watcher::Resources::ServiceActivity::reportDate, serviceReportDate ) ) {
            if( tmpJson[ "running_status" ].dump() == "false" ) {
                LOGGER_WARN( "Setting service status 'stopped' for " + serviceName +
                             " because 'running_status' == false" );

                tmpJson[ "service_status" ] = Watcher::Resources::ServiceActivity::stopped;
            }
            else if( m_unresponsiveProcesses.count(
                         json_get_int( tmpJson, Watcher::Resources::ServiceActivity::pid ) ) ) {
                tmpJson[ "service_status" ] = Watcher::Resources::ServiceActivity::corrupt;
            }
        }
        if( json_get( tmpJson, Watcher::Resources::ServiceActivity::pid, pid ) ) {
            tmpJson[ "respawn_count" ] = respwanCountMap[ pid ];
            json_remove( tmpJson, Watcher::Resources::ServiceActivity::pid );
        }

        serviceJson[ serviceName ] = tmpJson;
    }

    serviceReportData = JSON( Watcher::Resources::ServiceActivity::service << serviceJson );
    return true;
}

Types::ZWaveNodeStatus ServiceActivity::getZWaveNodeStatus() {
    return m_lastZWaveNodeStatus;
}

}  // namespace Watcher
}  // namespace HubPlatform
