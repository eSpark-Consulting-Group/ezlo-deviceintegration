#include "vhub_manager/vhub_manager.h"
#include "vhub_manager/resources.h"
#include "vhub_manager/vhub_manager_exception.h"
#include "vhub_manager/mqtt_configuration.h"

#include "mqtt_client/mqtt_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "system_utils/environment.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <unistd.h>
#include <map>

namespace HubPlatform {
namespace VhubManager {

namespace {
const std::string provisioningDataPath = Environment::getDataFolder() + "/factory.dat";
const std::string provisioningTmpPath = Environment::getProvisionDataFolder() + "/factory.dat";
}  // namespace

void VhubManager::startFirmware() {
    std::string command = Environment::getBinariesFolder() + "/start.sh";
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    if( result.second != 0 ) {
        throw FirmwareStartError( "firmware start error: code = " + std::to_string( result.second ) + " : message " +
                                  result.first );
    }
}

void VhubManager::stopFirmware() {
    std::string command = Environment::getBinariesFolder() + "/stop.sh";
    auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    if( result.second != 0 ) {
        throw FirmwareStartError( "firmware stop error: code = " + std::to_string( result.second ) + " : message " +
                                  result.first );
    }
}

Json VhubManager::getProvisionDataInfo() {
    if( !m_provisionData )
        throw ProvisionDataError( "Provision data not ready" );
    return JSON( "serial" << m_provisionData->id << "uuid" << m_provisionData->uuid );
}

void VhubManager::changeStatus( Status status ) {
    if( m_status != status ) {
        m_status = status;
        postCurrentStatus();
    }
}

std::string VhubManager::statusToString( Status status ) {
    const static std::map< Status, std::string > statusMap{ { Status::Unknown, "unknown" },
                                                            { Status::FirmwareStarted, "firmware_started" },
                                                            { Status::FirmwareStopped, "firmware_stopped" },
                                                            { Status::ProvisionDataNotFound,
                                                              "provision_data_not_found" } };
    return statusMap.at( status );
}

void VhubManager::postCurrentStatus() {
    using namespace HubPlatform::VhubManager::Resources;
    try {
        auto method = prepareMethod( RPC::vhubStatusChanged );
        method[ "params" ] = JSON( "status" << statusToString( m_status ) );
        m_mqttClient.publish( Common::MQTTUtils::createMessage( MQTT::provisionPageTopic, method.dump() ) );
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "Failed to post vhub status: " + CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        Log::warn( cc::warn( "Failed to post vhub status: unknown exception" ) );
    }
}

void VhubManager::clearVhubData() {
    if( !filesystem::clearFolder( Environment::getDataFolder() ) )
        Log::warn( cc::warn( "Failed to clean data dir" ) );
    if( !filesystem::removeFile( provisioningTmpPath.c_str() ) )
        Log::warn( cc::warn( "Failed to delete provision file: " + provisioningTmpPath ) );
    if( !filesystem::createFolder( Environment::getPersistentFolder() + "/data" ) )
        Log::warn( cc::warn( "Failed to create data dir" ) );
}

void VhubManager::prepareProvisioning() {
    auto isRegularFile = []( const std::string& path ) { return filesystem::isFileExist( path ); };

    if( filesystem::isExist( provisioningTmpPath ) ) {
        if( !isRegularFile( provisioningTmpPath ) )
            throw ProvisionDataError( "Provisioning is not a regular file: " + provisioningTmpPath );
        Log::debug( cc::debug( "Provisioning file found: " + provisioningTmpPath ) );
    }
    else if( filesystem::isExist( provisioningDataPath ) ) {
        if( !isRegularFile( provisioningDataPath ) )
            throw ProvisionDataError( "Provisioning is not a regular file: " + provisioningDataPath );

        if( !filesystem::copyFile( provisioningDataPath, provisioningTmpPath ) )
            throw ProvisionDataError( "Copy provisioning file error" );
        Log::debug( cc::debug( "Provisioning file copied to: " + provisioningTmpPath ) );
    }
    else {
        throw ProvisionDataError( "Provisioning file not found" );
    }
}

void VhubManager::processNewProvisioning( Json provisioningJson ) {
    using namespace ProvisionDataUtils;

    ProvisionData provisionData;
    provisionData.fillFromJson( provisioningJson );
    ProvisionDataManager().init( provisionData );

    if( !filesystem::isExist( provisioningTmpPath ) )
        throw ProvisionDataError( "Failed to create new provisioning file: " + provisioningTmpPath );

    if( !filesystem::copyFile( provisioningTmpPath, provisioningDataPath ) )
        throw ProvisionDataError( "Copy new provisioning file error" );
}

void VhubManager::init() {
    using namespace ProvisionDataUtils;
    try {
        mqttConnect();

        prepareProvisioning();
        m_provisionData = std::make_unique< ProvisionData >( loadProvisionData() );
        startFirmware();
        Log::info( cc::info( "Firmware started" ) );
        changeStatus( Status::FirmwareStarted );
    }
    catch( const ProvisionDataError& e ) {
        Log::error( cc::error( "Failed to init: provision data error: " ) + cc::note( CommonUtils::whatError( e ) ) );
        changeStatus( Status::ProvisionDataNotFound );
    }
    catch( const std::exception& e ) {
        Log::error( cc::error( "Failed to init: " + CommonUtils::whatError( e ) ) );
        changeStatus( Status::Unknown );
    }
    catch( ... ) {
        Log::error( cc::error( "Failed to init: unknown exception" ) );
    }
}

ProvisionDataUtils::ProvisionData VhubManager::loadProvisionData() {
    return ProvisionDataUtils::ProvisionDataManager().getProvisionData();
}

void VhubManager::mqttConnect() {
    using namespace HubPlatform::VhubManager;
    using namespace HubPlatform::Common;

    try {
        std::string clientId = "vhub_manager_" + std::to_string( getpid() );

        auto mqttCallback = [this]( Common::MQTTMessage&& msg ) {
            m_taskLoop.addTask( [this, msg]() { handleMQTTMessage( msg ); } );
        };

        if( !m_mqttClient.isConnected() ) {
            const char* mqtt_ip = MqttConfiguration::getMqttIP();
            int mqtt_port = MqttConfiguration::getMqttPort();
            const char* mqtt_user = MqttConfiguration::getMqttUser();
            const char* mqtt_password = MqttConfiguration::getMqttPassword();

            Log::info( cc::info( "Mqtt IP " ) + cc::note( mqtt_ip ) + cc::info( ":" ) +
                       cc::note( std::to_string( mqtt_port ) ) + cc::info( " USER " ) + cc::note( mqtt_user ) );

            m_mqttClient.connect( clientId,
                                  std::string( mqtt_ip ) + ":" + std::to_string( mqtt_port ),
                                  mqtt_user,
                                  mqtt_password,
                                  std::move( mqttCallback ) );
        }

        m_mqttClient.subscribe( Resources::MQTT::vhubManagerTopic );
        Log::info( cc::info( "MQTT client connected" ) );

        postCurrentStatus();
        return;
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "Failed to setup MQTT connection: " + CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        Log::warn( cc::warn( "Failed to setup MQTT connection: unknown exception" ) );
    }
    m_taskLoop.createPostponedTask( [this]( Services::PostponedTask& ) { mqttConnect(); } )
        ->start( std::chrono::seconds( 10 ), false );
}

void VhubManager::handleMQTTMessage( const Common::MQTTMessage& msg ) {
    using namespace HubPlatform::VhubManager::Resources;

    Json jsonMsg;
    std::string method;

    try {
        jsonMsg = parseJson( Common::MQTTUtils::getStrPayload( msg ) );
        method = json_get_str( jsonMsg, "method" );
        if( method.empty() )
            throw VhubManagerMessageError( "method is not set" );

        if( !json_contains( jsonMsg, "params" ) )
            throw VhubManagerMessageError( "params is not set" );

        if( method == RPC::provisionInfoGet ) {
            handleProvisionInfoGet( jsonMsg );
        }
        else if( method == RPC::provisionDataNew ) {
            handleProvisionDataNew( jsonMsg );
        }
        else if( method == RPC::provisionDataDelete ) {
            handleProvisionDataDelete( jsonMsg );
        }
        else if( method == RPC::vhubFirmwareStop ) {
            handleVhubFirmwareStop( jsonMsg );
        }
        else if( method == RPC::vhubFirmwareStart ) {
            handleVhubFirmwareStart( jsonMsg );
        }
        else if( method == RPC::vhubStatusGet ) {
            handleVhubStatusGet( jsonMsg );
        }
        else {
            Log::warn( cc::warn( "Failed to handle MQTT message: unknown method: " + method ) );
            setMsgError( jsonMsg, "unknown method" );
        }
    }
    catch( const VhubManagerMessageError& e ) {
        Log::warn( cc::warn( "Failed to handle method: " + method + ": " + CommonUtils::whatError( e ) ) );
        setMsgError( jsonMsg, "invalid format: " + std::string( e.what() ) );
    }
    catch( const ProvisionDataError& e ) {
        Log::warn(
            cc::warn( "Failed to handle method: " + method + ": provision error: " + CommonUtils::whatError( e ) ) );
        setMsgError( jsonMsg, "provision data error: " + std::string( e.what() ) );
        changeStatus( Status::ProvisionDataNotFound );
    }
    catch( const FirmwareStopError& e ) {
        Log::warn( cc::warn( "Failed to handle method: " + method +
                             ": firmware stop error: " + CommonUtils::whatError( e ) ) );
        setMsgError( jsonMsg, "firmware stop error" );
        changeStatus( Status::Unknown );
    }
    catch( const FirmwareStartError& e ) {
        Log::warn( cc::warn( "Failed to handle method: " + method +
                             ": firmware start error: " + CommonUtils::whatError( e ) ) );
        setMsgError( jsonMsg, "firmware start error" );
        changeStatus( Status::FirmwareStopped );
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "Failed to handle method: " + method + ": " + CommonUtils::whatError( e ) ) );
        setMsgError( jsonMsg, "unknown error" );
        changeStatus( Status::Unknown );
    }
    catch( ... ) {
        Log::warn( cc::warn( "Failed to handle MQTT message: unknown exception" ) );
        setMsgError( jsonMsg, "unknown error" );
        changeStatus( Status::Unknown );
    }

    try {
        LOGGER_TRACE_RAW( "Response message: " + jsonMsg.dump() );
        m_mqttClient.publish( Common::MQTTUtils::createMessage( MQTT::provisionPageTopic, jsonMsg.dump() ) );
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "Failed to response: " + CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        Log::warn( cc::warn( "Failed to response: unknown exception" ) );
    }
}

void VhubManager::handleProvisionInfoGet( Json& msg ) {
    auto info = getProvisionDataInfo();
    setMsgResponse( msg, std::move( info ) );
}

void VhubManager::handleProvisionDataNew( Json& msg ) {
    using namespace ProvisionDataUtils;

    Json params;
    json_get( msg, "params", params );

    Json provisionData;
    json_get( params, "provision_data", provisionData );

    stopFirmware();

    clearVhubData();
    processNewProvisioning( provisionData );
    prepareProvisioning();
    m_provisionData = std::make_unique< ProvisionData >( loadProvisionData() );

    startFirmware();

    setMsgResponse( msg, getProvisionDataInfo() );
    changeStatus( Status::FirmwareStarted );
}

void VhubManager::handleProvisionDataDelete( Json& msg ) {
    stopFirmware();
    clearVhubData();
    m_provisionData.reset();
    setMsgResponse( msg, JSON_EMPTY_OBJ );
    changeStatus( Status::ProvisionDataNotFound );
}

void VhubManager::handleVhubFirmwareStop( Json& msg ) {
    stopFirmware();
    setMsgResponse( msg, JSON_EMPTY_OBJ );
    if( m_status != Status::ProvisionDataNotFound )
        changeStatus( Status::FirmwareStopped );
}

void VhubManager::handleVhubFirmwareStart( Json& msg ) {
    using namespace ProvisionDataUtils;
    prepareProvisioning();
    m_provisionData = std::make_unique< ProvisionData >( loadProvisionData() );
    startFirmware();
    setMsgResponse( msg, JSON_EMPTY_OBJ );
    changeStatus( Status::FirmwareStarted );
}

void VhubManager::handleVhubStatusGet( Json& msg ) {
    setMsgResponse( msg, JSON( "status" << statusToString( m_status ) ) );
}

void VhubManager::setMsgResponse( Json& msg, Json response ) {
    msg[ "response" ] = std::move( response );
    json_remove( msg, "params" );
}

void VhubManager::setMsgError( Json& msg, std::string errorMsg, int code ) {
    msg[ "error" ] = JSON( "message" << errorMsg << "code" << code );
    json_remove( msg, "params" );
}

void VhubManager::run() {
    init();
    m_taskLoop.run();
}

}  // namespace VhubManager
}  // namespace HubPlatform
