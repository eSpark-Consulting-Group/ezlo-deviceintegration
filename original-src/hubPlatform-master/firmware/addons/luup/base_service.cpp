#include "luup/base_service.h"
#include "luup/vera_plugins/plugins_manager.h"

#include "addon_interfaces/addon.h"
#include "service_interfaces/groups_names.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "bus/messagesclasses.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/cpp14.h"
#include "base_utils/ezlodb.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/environment.h"

#include <cassert>

const std::string BaseService::DataRequestDir = "/data_request";
const std::string BaseService::LuaDir = "/luvd/";

const ServiceResponse ErrorResponse( ResponseFormat::TEXT, "ERROR" );
const ServiceResponse OkResponse( ResponseFormat::TEXT, "OK" );

const std::string HageviceSid = "urn:micasaverde-com:serviceId:HomeAutomationGateway1";  // HAGEVICE_SID from Vera

BaseService::BaseService( HubPlatform::Addons::Addon& addon, const std::string& port, const std::string& webRoot )
: m_port( port )
, m_webRoot( webRoot )
, m_addon( addon )
, m_userData( m_addon.getDBClient() )
, m_config() {}

const std::string& BaseService::getPort() const {
    return m_port;
}

const std::string& BaseService::getWebRoot() const {
    return m_webRoot;
}

ServiceResponse BaseService::handleDataRequest( const Query& query ) {
    const auto reqIdIt = query.find( "id" );
    if( reqIdIt != query.end() ) {
        const auto handler = getRequestHandler( reqIdIt->second );
        return ( this->*handler )( query );
    }

    return ErrorResponse;
}

ServiceResponse BaseService::staticDataRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::userDataRequestHandler( const Query& query ) {
    std::string userDataVersionValue;
    std::string noStaticUserDataValue;
    Json userData = JSON_EMPTY_OBJ;

    getQueryValue( query, "ns", noStaticUserDataValue );

    bool needStaticUserData = noStaticUserDataValue.empty() || noStaticUserDataValue != std::string{ "1" };

    if( getQueryValue( query, "DataVersion", userDataVersionValue ) ) {
        try {
            auto userDataVersion = std::stoll( userDataVersionValue );
            userData = m_userData.getData( userDataVersion, needStaticUserData );
        }
        catch( const std::exception& exception ) {
            Log::warn( "Luup::BaseService: fail to convert string to int" );
        }
    }
    else
        userData = m_userData.getData( 0, needStaticUserData );

    if( userData.empty() )
        return ServiceResponse{ ResponseFormat::TEXT, "NO_CHANGES" };

    return ServiceResponse{ ResponseFormat::JSON, userData.dump() };
}

ServiceResponse BaseService::actionsRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::invokeRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::sdataRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::sudRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::statusRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::jobStatusRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::cameraPortRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::deviceRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::roomRequestHandler( const Query& query ) {
    auto serviceResponse = ErrorResponse;

    const auto actionIt = query.find( "action" );
    if( actionIt != query.end() ) {
        if( actionIt->second == "create" ) {
            const auto nameIt = query.find( "name" );
            if( nameIt != query.end() ) {
                HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( m_addon );
                const auto response = coreDRemoteInterface.createRoom( nameIt->second );
                if( !hasError( response ) ) {
                    serviceResponse = OkResponse;
                }
            }
        }
        else if( actionIt->second == "rename" ) {
            const auto roomIt = query.find( "room" );
            const auto nameIt = query.find( "name" );
            if( roomIt != query.end() && nameIt != query.end() ) {
                HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( m_addon );
                const auto response =
                    coreDRemoteInterface.setRoomName( getRoomId( std::stoi( roomIt->second ) ), nameIt->second );
                if( !hasError( response ) ) {
                    serviceResponse = OkResponse;
                }
            }
        }
        else if( actionIt->second == "delete" ) {
            const auto roomIt = query.find( "room" );
            if( roomIt != query.end() ) {
                HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( m_addon );
                const auto response = coreDRemoteInterface.deleteRoom( roomIt->second );
                if( !hasError( response ) ) {
                    serviceResponse = OkResponse;
                }
            }
        }
    }

    return serviceResponse;
}

ServiceResponse BaseService::sceneRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::irRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::deviceIpRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::fileRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::luaRequestHandler( const Query& query ) {
    auto serviceResponse = ErrorResponse;

    const auto codeIt = query.find( "code" );
    if( codeIt != query.end() ) {
        HubPlatform::Services::Message message;
        message.group = "addon.lua";
        message.msgClass = JSON_RPC;
        message.msgSubClass = CALL_METHOD;
        message.data = JSON_EMPTY_OBJ;
        message.data[ "method" ] = "executeVeraCode";
        message.data[ "params" ][ "code" ] = codeIt->second;
        m_addon.send( message );
        serviceResponse = OkResponse;
    }

    return serviceResponse;
}

ServiceResponse BaseService::actionRequestHandler( const Query& query ) {
    std::string serviceId;
    if( !getQueryValue( query, "serviceId", serviceId ) || ( serviceId != HageviceSid ) ) {
        return ServiceResponse( ResponseFormat::TEXT, "Invalid service" );
    }

    std::string actionName;
    if( !getQueryValue( query, "action", actionName ) ) {
        return ServiceResponse( ResponseFormat::TEXT, "Invalid action name" );
    }

    if( actionName == "CreatePlugin" ) {
        std::string pluginId;
        if( !getQueryValue( query, "PluginNum", pluginId ) ) {
            return ServiceResponse( ResponseFormat::TEXT, "Missed PluginNum" );
        }

        // Optional params.
        std::string version;
        getQueryValue( query, "Version", version );
        std::string aesKey;
        getQueryValue( query, "AesKey", aesKey );
        std::string stateVariables;
        getQueryValue( query, "StateVariables", stateVariables );

        PluginsManager manager( m_addon.getDBClient() );
        if( manager.installPlugin( pluginId, version, aesKey, stateVariables ) ) {
            return OkResponse;
        }
    }
    else if( actionName == "DeletePlugin" ) {
        std::string pluginId;
        if( !getQueryValue( query, "PluginNum", pluginId ) ) {
            return ServiceResponse( ResponseFormat::TEXT, "Missed PluginNum" );
        }
        PluginsManager manager( m_addon.getDBClient() );
        if( manager.removePlugin( pluginId ) ) {
            return OkResponse;
        }
    }

    else if( actionName == "GetUserData" ) {
        std::string userDataVersionValue;
        Json userData = JSON_EMPTY_OBJ;

        if( getQueryValue( query, "DataVersion", userDataVersionValue ) ) {
            try {
                auto userDataVersion = std::stoll( userDataVersionValue );
                userData = m_userData.getData( userDataVersion );
            }
            catch( const std::exception& exception ) {
                Log::warn( "Luup::BaseService: fail to convert string to int" );
            }
        }
        else
            userData = m_userData.getData();

        if( userData.empty() )
            return ServiceResponse{ ResponseFormat::TEXT, "NO_CHANGES" };

        return ServiceResponse{ ResponseFormat::JSON, userData.dump() };
    }

    return ErrorResponse;
}

ServiceResponse BaseService::variableSetRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::variableGetRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::variableClearRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::objectGetRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::findDeviceRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::childDeviceRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::resyncRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::updatePluginRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::wgetRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::findUpnpRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::requestImageRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::refreshCacheRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::archiveVideoRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::videoStreamRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::serveStreamRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::streamRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::audioStreamRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::bleScanResultRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::addAllertRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::delAlertRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::relayRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::holePunchingRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::camLightsRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::ipRequestsRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::masterRegisteringRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::slaveDataChangedRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::blackListIpRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::provisionRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::liveEnergyUsageRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::temperatureRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::crash1RequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::crash2RequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::readAldbRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::reloadRequestHandler( const Query& query ) {
    if( !system( ( HubPlatform::Environment::getBinariesFolder() + "/reload.sh &" ).c_str() ) )
        return OkResponse;

    return ErrorResponse;
}

ServiceResponse BaseService::purgeUpnpDevicesRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::aliveRequestHandler( const Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::localtimeRequestHandler( const Query& query ) {
    char timeBuf[ 128 ];

    auto now = time( NULL );
    if( now <= 0 ) {
        sprintf( timeBuf, "<Invalid date %li>", now );
    }
    else {
        tm timeInfo;
        localtime_r( &now, &timeInfo );
        sprintf( timeBuf,
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 timeInfo.tm_year + 1900,
                 timeInfo.tm_mon + 1,
                 timeInfo.tm_mday,
                 timeInfo.tm_hour,
                 timeInfo.tm_min,
                 timeInfo.tm_sec );
    }

    return ServiceResponse( ResponseFormat::TEXT, timeBuf );
}

ServiceResponse BaseService::dummyRequestHandler( const BaseService::Query& query ) {
    return ErrorResponse;
}

ServiceResponse BaseService::backupCreateRequestHandler( const Query& query ) {
    auto serviceResponse = ErrorResponse;
    const auto response = m_addon.sendRequest( "backupd", JSON_EMPTY_OBJ, BACKUP_CREATE );
    if( !hasError( response ) ) {
        serviceResponse = OkResponse;
    }
    return serviceResponse;
}

ServiceResponse BaseService::backupRestoreRequestHandler( const Query& query ) {
    auto serviceResponse = ErrorResponse;
    auto request = JSON_EMPTY_OBJ;
    const auto restoreModuleIt = query.find( "index" );
    if( restoreModuleIt != query.end() ) {
        request[ "index" ] = restoreModuleIt->second;
    }
    const auto response = m_addon.sendRequest( "backupd", request, BACKUP_RESTORE );
    if( !hasError( response ) ) {
        serviceResponse = OkResponse;
    }
    return serviceResponse;
}

ServiceResponse BaseService::backupGetListRequestHandler( const BaseService::Query& query ) {
    auto serviceResponse = ErrorResponse;
    const auto response = m_addon.sendRequest( "backupd", JSON_EMPTY_OBJ, BACKUP_LIST );
    if( !hasError( response ) ) {
        serviceResponse = ServiceResponse( ResponseFormat::TEXT, response.dump() );
    }
    return serviceResponse;
}

bool BaseService::getQueryValue( const BaseService::Query& query, const std::string& key, std::string& value ) {
    auto it = query.find( key );
    if( it != query.end() ) {
        value = it->second;
        return true;
    }
    return false;
}

ResponseFormat BaseService::getResponseFormat( const Query& query ) {
    const auto it = query.find( "output_format" );
    if( it == query.end() ) {
        return ResponseFormat::TEXT;
    }

    const auto& outputFormat = it->second;
    if( outputFormat == "xml" ) {
        return ResponseFormat::XML;
    }
    else if( outputFormat == "json" ) {
        return ResponseFormat::JSON;
    }

    return ResponseFormat::TEXT;
}

ServiceResponse BaseService::setHouseModeRequestHandler( const Query& query ) {
    std::string serviceId;
    if( !getQueryValue( query, "serviceId", serviceId ) || ( serviceId != HageviceSid ) ) {
        return ServiceResponse( ResponseFormat::TEXT, "Invalid service" );
    }

    std::string actionName;
    if( !getQueryValue( query, "action", actionName ) ) {
        return ServiceResponse( ResponseFormat::TEXT, "Invalid action name" );
    }

    if( actionName == "SetHouseMode" ) {
        std::string mode;
        getQueryValue( query, "Mode", mode );

        HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( m_addon );

        Log::debug( "BaseService::setHouseModeRequestHandler house mode: " + mode );
        auto result = remoteInterface.setHouseMode( mode );

        return !hasError( result ) ? OkResponse : ErrorResponse;
    }
    return ErrorResponse;
}

BaseService::RequestHandler BaseService::getRequestHandler( const std::string& id ) {
    static const RequestHandlersMap requestHandlers = {
        { "static", &BaseService::staticDataRequestHandler },
        { "user_data", &BaseService::userDataRequestHandler },
        { "user_data2", &BaseService::userDataRequestHandler },
        { "actions", &BaseService::actionsRequestHandler },
        { "invoke", &BaseService::invokeRequestHandler },
        { "sdata", &BaseService::sdataRequestHandler },
        { "sud", &BaseService::sudRequestHandler },
        { "status", &BaseService::statusRequestHandler },
        { "status2", &BaseService::statusRequestHandler },
        { "jobstatus", &BaseService::jobStatusRequestHandler },
        { "camera_port", &BaseService::cameraPortRequestHandler },
        { "device", &BaseService::deviceRequestHandler },
        { "room", &BaseService::roomRequestHandler },
        { "scene", &BaseService::sceneRequestHandler },
        { "ir", &BaseService::irRequestHandler },
        { "deviceip", &BaseService::deviceIpRequestHandler },
        { "file", &BaseService::fileRequestHandler },
        { "lua", &BaseService::luaRequestHandler },
        { "action", &BaseService::actionRequestHandler },
        { "variableset", &BaseService::variableSetRequestHandler },
        { "variableget", &BaseService::variableGetRequestHandler },
        { "variableclear", &BaseService::variableClearRequestHandler },
        { "objectget", &BaseService::objectGetRequestHandler },
        { "finddevice", &BaseService::findDeviceRequestHandler },
        { "childdevices", &BaseService::childDeviceRequestHandler },
        { "resync", &BaseService::resyncRequestHandler },
        { "update_plugin", &BaseService::updatePluginRequestHandler },
        { "wget", &BaseService::wgetRequestHandler },
        { "find_upnp", &BaseService::findUpnpRequestHandler },
        { "request_image", &BaseService::requestImageRequestHandler },
        { "refresh_cache", &BaseService::refreshCacheRequestHandler },
        { "archive_video", &BaseService::archiveVideoRequestHandler },
        { "video_stream", &BaseService::videoStreamRequestHandler },
        { "serve_stream", &BaseService::serveStreamRequestHandler },
        { "stream", &BaseService::streamRequestHandler },
        { "audio_stream", &BaseService::audioStreamRequestHandler },
        { "ble_scan_result", &BaseService::bleScanResultRequestHandler },
        { "add_alert", &BaseService::addAllertRequestHandler },
        { "del_alert", &BaseService::delAlertRequestHandler },
        { "relay", &BaseService::relayRequestHandler },
        { "hole_punching", &BaseService::holePunchingRequestHandler },
        { "camlights", &BaseService::camLightsRequestHandler },
        { "iprequests", &BaseService::ipRequestsRequestHandler },
        { "master_registering", &BaseService::masterRegisteringRequestHandler },
        { "slave_data_changed", &BaseService::slaveDataChangedRequestHandler },
        { "blacklistip", &BaseService::blackListIpRequestHandler },
        { "provision", &BaseService::provisionRequestHandler },
        { "live_energy_usage", &BaseService::liveEnergyUsageRequestHandler },
        { "temperature", &BaseService::temperatureRequestHandler },
        { "crash1", &BaseService::crash1RequestHandler },
        { "crash2", &BaseService::crash2RequestHandler },
        { "readaldb", &BaseService::readAldbRequestHandler },
        { "reload", &BaseService::reloadRequestHandler },
        { "purge_upnpdevices", &BaseService::purgeUpnpDevicesRequestHandler },
        { "alive", &BaseService::aliveRequestHandler },
        { "localtime", &BaseService::localtimeRequestHandler },
        { "hub.backup.create", &BaseService::backupCreateRequestHandler },
        { "hub.backup.restore", &BaseService::backupRestoreRequestHandler },
        { "hub.backup.get.list", &BaseService::backupGetListRequestHandler },
        { "lu_action", &BaseService::setHouseModeRequestHandler }

    };

    const auto handlerIt = requestHandlers.find( id );
    return ( handlerIt != requestHandlers.end() ) ? handlerIt->second : &BaseService::dummyRequestHandler;
}

std::string BaseService::getRoomId( int roomIdx ) {
    HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( m_addon );
    const auto response = remoteInterface.getAllRooms();

    std::string roomId;
    if( !hasError( response ) && response.is_array() && response.size() > static_cast< size_t >( roomIdx ) ) {
        roomId = Db::getId( response[ roomIdx ] );
    }

    return roomId;
}
