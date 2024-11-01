
#include "uid/proto.h"
#include "uid/nma_client.h"
#include "uid/ui_service.h"
#include "uid/firmware.h"
#include "uid/ezlo.h"
#include "uid/ruleblocks.h"
#include "uid/nma_proto.h"
#include "uid/broadcast.h"
#include "uid/wifi.h"
#include "uid/answers.h"
#include "uid/fo_converter.h"
#include "uid/environment.h"
#include "uid/response_builders/response_builders_factory.h"
#include "uid/response_builders/getdevices/iresponse_builder.h"
#include "uid/response_builders/getitems/iresponse_builder.h"
#include "uid/response_builders/device_settings_get/iresponse_builder.h"
#include "uid/handlers/hub_software_commands_handler.h"
#include "uid/handlers/hub_network_commands_handler.h"
#include "uid/handlers/hub_provision_info_commands_handler.h"
#include "uid/handlers/hub_addons_commands_handler.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "uid/handlers/hub_features_commands_handler.h"
#include "uid/handlers/hub_settings_commands_handler.h"
#include "uid/handlers/hub_cloud_subscriptions_handler.h"
#include "uid/handlers/hub_video_forward_handler.h"
#include "uid/handlers/hub_vidoo_commands_handler.h"
#include "uid/handlers/hub_protect_command_id_handler.h"
#include "uid/handlers/hub_protect_security_config_handler.h"
#include "uid/handlers/hub_notifications_handler.h"
#include "uid/response_builders/filter_data_list.h"
#include "uid/common_methods.h"

#include "cored/model/cored_device_setting.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/exceptions.h"
#include "cored/model/values/int_value.h"
#include "cored/model/values/zwave_configuration_value.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"
#include "cored/model/visitors/ui_value_writer_visitor.h"
#include "cored/model/visitors/from_ui_value_getter_visitor.h"
#include "backupd/remote_interface/backup_remote_interface.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "networkd/remote_interface/network_remote_interface.h"

#include "log_utils/ezlo_log.h"
#include "device_model/h/permission.h"
#include "base_utils/utils.h"
#include "base_utils/fo_utils.h"
#include "base_utils/ezlodb.h"
#include "base_utils/acl.h"
#include "base_utils/suffixes.h"
#include "base_utils/timeutils.h"
#include "system_utils/environment.h"
#include "json_utils/json_custom_getters.h"
#include "version/ha_version.h"
#include "bus/spread_bus.h"
#include "database_client/db_tables_names.h"
#include "common_utils/firmware_info_helper.h"
#include "service_interfaces/service.h"
#include "hub_settings/hub_settings_manager.h"

#include <tiny/tiny_process.h>
#include <tiny/tiny_url.h>
#include <tiny/tiny_utils.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <thread>
#include <iostream>
#include <fstream>
#include <exception>
#include <cassert>
#include <functional>
#include <tiny/tiny_process.h>
#include <tiny/tiny_url.h>
#include <tiny/tiny_utils.h>
#include "base_utils/suffixes.h"
#include "base_utils/timeutils.h"
#include "json_utils/json_custom_getters.h"
#include "uid/response_builders/hub_object_list_device_filter.h"
#include "uid/response_builders/getdevices/devices_list_builder.h"
#include "uid/response_builders/getitems/items_list_builder.h"
#include "system_utils/system.h"

#include <ctime>
#include <cerrno>

#if( defined KERN_BOOTTIME )
time_t stat_bsd_system_uptime() {
    struct timeval boottime;
    size_t len = sizeof( boottime );
    int mib[ 2 ] = { CTL_KERN, KERN_BOOTTIME };
    if( sysctl( mib, 2, &boottime, &len, NULL, 0 ) < 0 ) {
        return -1.0;
    }
    //    time_t bsec = boottime.tv_sec, csec = time(NULL);
    //    return (double) ( difftime(csec, bsec) );
    return boottime.tv_sec;
}
#else  // (defined KERN_BOOTTIME)
#include <sys/sysinfo.h>
#include <sys/utsname.h>

time_t stat_linux_system_uptime() {
    struct sysinfo si;
    if( !sysinfo( &si ) )
        return si.uptime;
    return 0;
}
#endif  // else from (defined KERN_BOOTTIME)

namespace HubPlatform {
namespace Uid {

Proto::Proto( Service& service, HubPlatform::ScenesManager::RemoteInterface& ri )
: m_uiService( service )
, m_scenesRemoteInterface( ri ) {}

bool Proto::processCmd( rpc::Message& rpcMessage ) {
    const JsonObject& joMessage = rpcMessage.getMessage();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string method = rpcMessage.getMethod();

    if( method.empty() ) {
        if( json_get_str( joMessage, "id" ) != "ui_broadcast" || json_get_str( joMessage, "msg_subclass" ).empty() ) {
            setError( joAnswer, Errors::MESSAGE_HAS_EMPTY_METHOD );
            Log::fatal( cc::fatal( "\"method\" is empty in JSON (UI proto)" ) );
        }
        return false;
    }
    std::string id = rpcMessage.getId();
    if( id.empty() )
        Log::warn( cc::warn( "\"id\" is empty in JSON for method \"" + method + "\" (UI proto)" ) );
    JsonObject joSender = JSON_EMPTY_OBJ;
    std::string stringSenderId;
    json_add_null( joAnswer, Errors::ERROR_FIELD );
    if( json_get_obj( joMessage, "sender", joSender ) ) {
        joAnswer[ "sender" ] = joSender;
    }
    else if( json_get( joMessage, "sender", stringSenderId ) ) {
        joAnswer[ "sender" ] = stringSenderId;
    }

    if( !id.empty() )
        json_add_str( joAnswer, "id", id );

    if( !hubLogCommand( id, method, joMessage ) && !cloudLogCommand( id, method, joMessage, joAnswer ) ) {
        Log::info( cc::info( "Received new command: " ) + cc::note( method ) + cc::info( ", id: " ) + cc::note( id ) );
    }

    if( joMessage.count( "result" ) > 0 && joMessage[ "result" ].type() == Json::value_t::object )
        joAnswer[ "result" ] = joMessage[ "result" ];
    else
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;

    auto isExecuted = invokeHandler( method, rpcMessage );
    if( !isExecuted ) {
        auto submethodPos = method.find( '.' );
        if( submethodPos != std::string::npos ) {
            const auto methodId = method.substr( 0, submethodPos );
            const auto submethodId = method.substr( submethodPos + 1 );
            if( !methodId.empty() && !submethodId.empty() ) {
                isExecuted = invokeExtendedHandler( methodId, submethodId, rpcMessage );
            }
        }
    }

    if( !isExecuted ) {
        isExecuted = redirectCustomServiceMessage( rpcMessage );
    }

    if( !isExecuted ) {
        unknown( rpcMessage );
        Log::warn( cc::warn( "No handler was found for the command: " ) + cc::note( method ) );
    }

    return isExecuted;
}

void Proto::processBroadcast( const Services::Message& /*message*/ ) {}

void Proto::unknown( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

void Proto::ping( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    json_add_str( joAnswer, "reply", "Pong" );
}

void Proto::setDescription( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string description;
    if( !getField( joParams, "description", joAnswer, description ) ) {
        Log::debug( "Proto::setDescription no description" );
        return;
    }
    Ezlo::setDescription( description );
}

void Proto::getWifiList( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        &Wifi::getList,
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::connectWifi( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        std::bind( &Wifi::connect, joParams, std::placeholders::_1 ),
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::deleteWifi( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        std::bind( &Wifi::del, joParams, std::placeholders::_1 ),
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::getStatusWifi( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        std::bind( &Wifi::getStatus, joParams, std::placeholders::_1 ),
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::resetWifi( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        &Wifi::reset,
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::getEzloIp( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    Wifi::run(
        &Wifi::getIp,
        std::bind( []( std::string s,
                       rpc::Message::GenericSenderPtr pNetworkObject ) -> void { pNetworkObject->sendMessage( s ); },
                   std::placeholders::_1,
                   rpcMessage.getNetworkObject() ),
        joAnswer );
    joAnswer = nullptr;
}

void Proto::getEzloMac( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer[ "result" ] = Ezlo::getMacs();
}

void Proto::changeItemValueLevelByPercent( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    using namespace CoreD::Model::Resources;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface interface( m_uiService );

        const auto& params = rpcMessage.getParams();
        auto& error = rpcMessage.getAnswer()[ std::string( Errors::ERROR_FIELD ) ];

        const auto itemId = json_get_str( params[ std::string( ItemFields::IdFieldName ) ] );
        if( itemId.empty() ) {
            Errors::makeObject(
                Errors::Codes::INVALID_REQUEST, "rpc.params.empty._id", "Bad request, _id is empty", error );
            return;
        }

        const auto notFoundValue = std::numeric_limits< int >::max();
        const auto percent = json_get_int( params, "percent", notFoundValue );
        if( percent == notFoundValue ) {
            Errors::makeObject(
                Errors::Codes::INVALID_PARAMS, "rpc.params.type.invalid", "Value has invalid type", error );
            return;
        }

        if( percent < -100 || percent > 100 ) {
            Errors::makeObject(
                Errors::Codes::INVALID_PARAMS, "rpc.params.range.invalid", "Value is not in valid range", error );
            return;
        }

        const auto resultValueUPtr = interface.getItemValueInMinMaxRange( itemId, percent );
        if( resultValueUPtr ) {
            // Now we don't know what the type of a value and are using Value::toJson()
            //   virtual function to load response with data
            CoreD::Model::Value& someValue = *resultValueUPtr.get();

            JsonObject tmpJson;
            CoreD::Model::UiValueWriterVisitor valueGetter( tmpJson, interface.getConfiguration() );
            valueGetter.writeValue( someValue, "value" );

            // Iterate all fields of tmpJson, add key-value pairs to message.params.
            if( tmpJson.is_object() ) {
                for( auto it = tmpJson.begin(), eit = tmpJson.end(); it != eit; it++ ) {
                    rpcMessage.setParamsField( it.key(), it.value() );
                }
            }
            else {
                rpcMessage.setParamsField( ItemFields::ValueFieldName, tmpJson );
            }

            auto& joAnswer = rpcMessage.getAnswer();
            auto peer = rpcMessage.getNetworkObject();
            interface.setItemValue( params, [joAnswer, tmpJson, peer]( const JsonObject& response ) mutable {
                setErrorIfHas( joAnswer, response );
                joAnswer[ "result" ] = std::move( response );

                // Iterate all fields of tmpJson, add key-value pairs to message.result.
                // For 'result', 'value' field should be stored with 'newValue' key.
                if( tmpJson.is_object() ) {  // updates answer.result
                    auto& resultCollection = joAnswer[ "result" ];
                    for( auto it = tmpJson.begin(), eit = tmpJson.end(); it != eit; it++ ) {
                        if( it.key() == "value" ) {
                            resultCollection[ "newValue" ] = it.value();
                        }
                        else {
                            resultCollection[ it.key() ] = it.value();
                        }
                    }
                }

                json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                if( peer ) {
                    peer->sendMessage( joAnswer );
                }
            } );
            joAnswer = nullptr;
        }
        else {
            Errors::makeObject( Errors::Codes::INVALID_PARAMS,
                                "rpc.params.type.invalid",
                                "Currently only items with type `int` are supported",
                                error );
        }
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
}

void Proto::setEzloLogLevel( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string logLevel;
    if( getOptionalField( joParams, "logLevel", logLevel ) ) {
        json_add_str( rpcMessage.getParams(), "severity", logLevel );
        setEzloLogSettings( rpcMessage );
    }
    else {
        setError( joAnswer, Errors::WRONG_PARAMS );
    }
}

void Proto::setEzloLogSettings( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string logLevel, logLevelPrev = Log::toString( static_cast< Log::PriorityLevel >( Log::getLogLevel() ) );
    bool useColors = Log::getLocalColors(), useColorsPrev = Log::getLocalColors();
    int indent = Log::getLocalIndent(), indentPrev = Log::getLocalIndent();

    auto checkSeverity = [&joAnswer, &joParams]() {
        Json error;
        auto foundIt = joParams.find( "severity" );
        if( foundIt != joParams.end() && foundIt->is_string() ) {
            const std::string& severity = toUpper( json_get_str( *foundIt ) );
            static std::vector< std::string > levels = { "ERROR", "WARNING", "INFO", "DEBUG", "TRACE" };
            auto checkSeverityValues = [&severity]( const std::string& level ) { return severity == level; };
            if( !std::any_of( begin( levels ), end( levels ), checkSeverityValues ) ) {
                setError(
                    joAnswer, Errors::Error::SEVERITY_DOES_NOT_EXIST, "The severity with this name does not exist" );
                return false;
            }
            return true;
        }
        Errors::getWrongParams( "severity", error );
        setErrorObject( joAnswer, error );
        return false;
    };

    if( !checkSeverity() )
        return;

    {  // optional
        auto foundIt = joParams.find( "color" );
        if( foundIt != joParams.end() && !foundIt->is_boolean() ) {
            Json error;
            Errors::getWrongParams( "color", error );
            setErrorObject( joAnswer, error );
            return;
        }
    }

    {  // optional
        auto foundIt = joParams.find( "indent" );
        if( foundIt != joParams.end() ) {
            bool wrongParam = false;
            if( foundIt->is_number_integer() ) {
                std::vector< int > goodValues = { -1, 2, 4, 8 };
                int indentValue = foundIt->get< int >();
                wrongParam = !std::any_of( begin( goodValues ), end( goodValues ), [indentValue]( int item ) -> bool {
                    return item == indentValue;
                } );
            }
            else {
                wrongParam = true;
            }

            if( wrongParam ) {
                Json error;
                Errors::getWrongParams( "indent", error );
                setErrorObject( joAnswer, error );
                return;
            }
        }
    }

    try {
        getOptionalField( joParams, "color", useColors );
        getOptionalField( joParams, "indent", indent );

        if( getOptionalField( joParams, "severity", logLevel ) ) {
            logLevel = toUpper( logLevel );
            if( logLevel != logLevelPrev || useColors != useColorsPrev || indent != indentPrev ) {
                using namespace Services::Resources;
                CoreDRemoteInterface::CoreDRemoteInterface coreDRemote( getService() );
                if( logLevel != logLevelPrev ) {
                    auto hubSetting = coreDRemote.findSetting( HubSettings::LogsLevel );
                    if( hubSetting && hubSetting->setTokenValue( logLevel ) ) {
                        LOGGER_TRACE( cc::trace( "set new setting value: " ) + logLevel );
                        coreDRemote.saveSetting( *hubSetting );
                    }
                    else {
                        LOGGER_ERROR( cc::error( "setting not found or set value failed: " ) + HubSettings::LogsLevel );
                    }
                }

                if( useColors != useColorsPrev ) {
                    auto hubSetting = coreDRemote.findSetting( Services::Resources::HubSettings::LogsColor );
                    if( hubSetting && hubSetting->setBoolValue( useColors ) ) {
                        LOGGER_TRACE( cc::trace( "set new setting value: " ) + cc::flag( useColors ) );
                        coreDRemote.saveSetting( *hubSetting );
                    }
                    else {
                        LOGGER_ERROR( cc::error( "setting not found or set value failed: " ) + HubSettings::LogsLevel );
                    }
                }
                if( indent != indentPrev ) {
                    auto hubSetting = coreDRemote.findSetting( Services::Resources::HubSettings::LogsIndent );
                    if( hubSetting && hubSetting->setTokenValue( std::to_string( indent ) ) ) {
                        LOGGER_TRACE( cc::trace( "set new setting value: " ) + cc::num10( indent ) );
                        coreDRemote.saveSetting( *hubSetting );
                    }
                    else {
                        LOGGER_ERROR( cc::error( "setting not found or set value failed: " ) + HubSettings::LogsLevel );
                    }
                }

                coreDRemote.reloadGlobalLogOptions();
            }
        }
        else {
            setError( joAnswer, Errors::WRONG_PARAMS );
        }
    }
    catch( std::exception& ex ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getWrongParams( "severity", error );
        setErrorObject( joAnswer, error, "Format of any required field is wrong" );
    }
}

/** Run command in a separate thread, report command result back in async way */
void Proto::hubLogSend( rpc::Message& rpcMessage ) {
    using PostponedTask = HubPlatform::Services::PostponedTask;

    const auto messageId = rpcMessage.getId();
    const JsonObject& joParams = rpcMessage.getParams();
    auto& joAnswer = rpcMessage.getAnswer();
    auto peer = rpcMessage.getNetworkObject();

    try {
        // Compose logcmd call
        std::stringstream logCmd;
        logCmd << HubPlatform::Environment::getBinariesFolder().append( "/logcmd" ) << " ";

        JsonArray select;
        json_get_arr( joParams, "select", select );
        if( !select.empty() ) {
            logCmd << "--select=";
            for( const auto& element : select ) {
                logCmd << json_get_str( element ) << ",";
            }
        }
        else if( json_get_bool( joParams, "rotate", false ) == true ) {
            LOGGER_INFO( "Log send with rotate the whole logs collection, traceId=" + messageId );
            logCmd << " "
                   << "--rotate --force-upload --add-to-name=log_send_rotate_command";
        }

        if( Log::getLogLevel() == Log::TRACE )
            logCmd << " --log-level=TRACE";
        else if( Log::getLogLevel() == Log::DEBUG )
            logCmd << " --log-level=DEBUG";

        if( !messageId.empty() ) {
            logCmd << " --trace-id=" << messageId;
        }
        auto cmdStr = logCmd.str();
        logCmd.clear();

        auto future = std::async( std::launch::async, [this, cmdStr, messageId, joAnswer, peer]() mutable {
            // Run logcmd synchronously.
            LOGGER_DEBUG_RAW( cc::debug( "Start uploading logs to cloud, traceId=" + messageId ) );
            LOGGER_DEBUG_RAW( cc::debug( "Log command: " + cmdStr ) );

            auto result = System::getCommandOutput( cmdStr.c_str(), true, false );
            if( result.second == 0 ) {
                LOGGER_DEBUG_RAW( cc::debug( "Logs have been uploaded to cloud, traceId=" + messageId ) );
            }
            else {
                auto exitCode = System::unpackExitCode( result.second ).exitCode;
                LOGGER_WARN_RAW( cc::warn( "Failed to upload logs to cloud, exit code: " ) +
                                 std::to_string( exitCode ) + ", traceId=" + messageId );
                setError( joAnswer,
                          Errors::SEND_LOGS_ERROR,
                          "Failed to upload logs with code " + std::to_string( exitCode ) );
            }
            Json logCmdOutput = {
                { "commandOutput", result.first }, { "traceId", messageId }, { "error", joAnswer[ "error" ] }
            };
            LOGGER_DEBUG_RAW( "Logcmd output: " + cc::j( logCmdOutput ) );

            // Return result to the taskloop thread, and notify peer about the command result.
            this->m_uiService.postTask( [peer, joAnswer, messageId]() mutable -> void {
                LOGGER_TRACE_RAW( "hub.log.send: Logs upload finished, return result to peer, traceId=" + messageId );
                if( peer ) {
                    peer->sendMessage( joAnswer );
                    peer.reset();
                }
                joAnswer = nullptr;
            } );
        } );

        joAnswer = nullptr;
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Exception caught during hub.log.send: " + std::string( e.what() ) ) +
                         ", traceId=" + messageId );
        setError( joAnswer, Errors::SEND_LOGS_ERROR );
    }
}

void Proto::getEzloVersion( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    JsonObject result = JSON_EMPTY_OBJ;
    json_add_str( result, "ezlo_firmware_version", FirmwareInfoHelper::getFirmwareVersionString() );
    json_add_str( result, "ezlo_app_version", HubPlatform::Version::ha_version() );
    json_add_str( result, "description", Ezlo::getDescription() );
    json_add_str( result, "zwaveStickCalibration", Ezlo::getZwaveCalibration() );
    auto time_offset = Ezlo::getTimeOffset();  // l_sergiy: offset east of UTC in seconds
    json_add_int( result, "timeOffset", time_offset );
    json_add_str( result, "region", FirmwareInfoHelper::getFirmwareRegion() );
    json_add_str( result, "hubType", Ezlo::hubType() );

    unsigned long uptime =
#if( defined KERN_BOOTTIME )
        stat_bsd_system_uptime();
#else   // (defined KERN_BOOTTIME)
        stat_linux_system_uptime();
#endif  // else from (defined KERN_BOOTTIME)
    json_add_long( result, "uptime", uptime );
    time_t now;
    time( &now );
    now -= time_offset;  // l_sergiy: offset east of UTC in seconds
    struct tm tm_;
    localtime_r( &now, &tm_ );
    if( tm_.tm_isdst )
        now -= 3600;  // l_sergiy: ??? is it correct??? or should we use popular date_time_zonespec.csv database???
    char buffer_time_only[ 256 ];
    strftime( buffer_time_only,
              sizeof( buffer_time_only ),
              "%a, %d %b %Y %T",  // l_sergiy: time only, time zone will append separately
              gmtime_r( &now, &tm_ ) );
    int abs_time_offset = int( time_offset );
    if( abs_time_offset < 0 )
        abs_time_offset = -abs_time_offset;
    std::string strTime =
        tiny::tools::format( "%s %c%02d%02d",  // l_sergiy: about time zone formatting: ISO 8601 offset from UTC in
                                               // timezone (1 minute=1, 1 hour=100), example: +100
                             buffer_time_only,
                             ( time_offset <= 0 ) ? '+' : '-',  // l_sergiy: offset east of UTC in seconds
                             abs_time_offset / 3600,            // l_sergiy: hours
                             abs_time_offset % 3600             // l_sergiy: minutes
        );
    json_add_str( result, "current_time", strTime );
    json_add_long( result, "current_time_utc", static_cast< long >( now ) );
    joAnswer[ "result" ] = result;
}

void Proto::requestNewNMAServer( rpc::Message& /*rpcMessage*/ ) {
    m_uiService.addTask( [this]() -> void {
        m_uiService.getNmaClient()->closeConnection();
        m_uiService.getNmaClient()->resetServerUrl();
        m_uiService.getEnvironment()->invalidateNmaServerURL();
        m_uiService.getNmaClient()->restoreConnection();
    } );
}

JsonObject Proto::prepareMessage( JsonObject joParams, const std::string& operationId ) {
    joParams.erase( "sender" );
    joParams.erase( "permission" );
    joParams.erase( "serial" );
    joParams[ "operationId" ] = operationId;
    return joParams;
}

HubPlatform::ScenesManager::RemoteInterface& Proto::getScenesManager() {
    return m_scenesRemoteInterface;
}

void Proto::setItemValue( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( m_uiService );
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        coreDInterface.setItemValue( rpcMessage.getParams(), [joAnswer, peer]( const JsonObject& response ) mutable {
            setErrorIfHas( joAnswer, response );
            joAnswer[ "result" ] = std::move( response );
            json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );

            if( peer ) {
                peer->sendMessage( joAnswer );
            }
        } );
        rpcMessage.getAnswer() = nullptr;
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const CoreD::DeviceBusy& e ) {
        setError( rpcMessage.getAnswer(), Errors::DEVICE_BUSY, e.what() );
    }
    catch( const CoreD::Model::InvalidValueScale& e ) {
        setError( rpcMessage.getAnswer(), Errors::INVALID_VALUE_SCALE );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::getServiceInfo( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;

    try {
        auto serviceName = json_get_str( rpcMessage.getParams(), "service" );
        if( serviceName.empty() ) {
            setError( rpcMessage.getAnswer(), Errors::EMPTY_FIELD, "Field 'service' should not be empty" );
            return;
        }

        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();

        m_uiService.sendAsyncRequest( serviceName,
                                      rpcMessage.getParams(),
                                      SERVICE_INFO_GET,
                                      [joAnswer, peer]( const JsonObject& response ) mutable {
                                          setErrorIfHas( joAnswer, response );
                                          joAnswer[ "result" ] = response;
                                          json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                          if( peer ) {
                                              peer->sendMessage( joAnswer );
                                          }
                                      },
                                      std::chrono::seconds{ 30 } );
        joAnswer = nullptr;
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::invokeHubSoftwareHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubSoftwareCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void Proto::invokeNetworkHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubNetworkCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void Proto::invokeProvisionInfoHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubProvisionInfoCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void Proto::invokeHubAddonsHandler( const std::string& method, rpc::Message& rpcMessage ) {
    HubAddonsCommandsHandler( getService() ).invoke( method, rpcMessage );
}

void Proto::invokeHubBackupHandler( const std::string& method, rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    BackupUpdateManager::RemoteInterface backupManager( m_uiService );
    backupManager.processCmd( method, rpcMessage );
}

bool Proto::invokeHubHandler( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( Proto::* )( const std::string&, rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;

    auto submethodPos = method.find( '.' );
    if( submethodPos != std::string::npos ) {
        const auto methodId = method.substr( 0, submethodPos );
        const auto submethodId = method.substr( submethodPos + 1 );

        static const RequestHandlersMap handlers = {
            { "software", &Proto::invokeHubSoftwareHandler },
            { "network", &Proto::invokeNetworkHandler },
            { "provision_info", &Proto::invokeProvisionInfoHandler },
            { "addons", &Proto::invokeHubAddonsHandler },
            { "backup", &Proto::invokeHubBackupHandler },
            { "backups", &Proto::invokeHubBackupHandler },
        };

        const auto it = handlers.find( methodId );
        if( it != handlers.end() ) {
            const auto handler = it->second;
            ( this->*handler )( submethodId, rpcMessage );
            return true;
        }
    }
    return false;
}

namespace {
using DeviceAdditionalInfoArray = std::vector< HubPlatform::DeviceAdditionalInfo >;
using DeviceFirmwareInfoArray = std::vector< HubPlatform::FirmwareInfo >;
using ArmedState = Uid::ResponseBuilder::GetDevices::ArmedState;
using HouseModesOptions = Json;

std::tuple< DeviceAdditionalInfoArray, DeviceFirmwareInfoArray, ArmedState, HouseModesOptions >
prepareDeviceResponseFields( const JsonObject& device ) {
    using namespace CoreD::Model::Resources;

    DeviceAdditionalInfoArray additionalInfo;
    DeviceFirmwareInfoArray firmwareInfo;

    const JsonObject& joInfo = device[ DeviceFields::InfoFieldName ];
    if( joInfo.is_object() && !joInfo.empty() ) {
        additionalInfo.reserve( joInfo.size() );

        for( auto it = joInfo.begin(), end = joInfo.end(); it != end; ++it ) {
            const auto& joValue = it.value();

            std::string value = joValue.is_string() ? joValue.get< std::string >() : joValue.dump();
            additionalInfo.emplace_back( it.key(), std::move( value ) );
        }
    }

    const JsonObject firmwareJson = device[ DeviceFields::FirmwareFieldName ];
    if( firmwareJson.is_array() && !firmwareJson.empty() ) {
        firmwareInfo.reserve( firmwareJson.size() );

        for( const auto& firmwareInfoJson : firmwareJson ) {
            FirmwareInfo info{ json_get_str( firmwareInfoJson, DeviceFields::FirmwareIdFieldName ),
                               json_get_str( firmwareInfoJson, DeviceFields::FirmwareVersionFieldName ) };
            firmwareInfo.push_back( std::move( info ) );
        }
    }

    ArmedState armed = ArmedState::NoState;
    bool isArmed;
    if( json_get( device, DeviceFields::ArmedFieldName, isArmed ) ) {
        armed = isArmed ? ArmedState::Armed : ArmedState::Disarmed;
    }

    HouseModesOptions houseModesOptions;  // optional
    json_get_obj( device, DeviceFields::HouseModesOptionsFieldName, houseModesOptions );

    return std::make_tuple( std::move( additionalInfo ), std::move( firmwareInfo ), armed, houseModesOptions );
}

DataListFilterUPtr initDataListFilter( const JsonObject& params, const std::string& idsFieldName ) {
    auto ptr = makeDataListFilter();
    if( ptr == nullptr )
        throw std::runtime_error( "Could not create data.list filter" );

    auto& dataFilter = *ptr;
    JsonObject ids, fields, includeFields;
    if( json_get_arr( params, idsFieldName, ids ) ) {
        dataFilter.initIds( ids );
    }
    if( json_get_obj( params, "fields", fields ) && json_get_arr( fields, "include", includeFields ) ) {
        dataFilter.initIncludeFields( includeFields );
    }

    return ptr;
}
}  // namespace

void Proto::buildGetDevicesResponse( rpc::Message& rpcMessage, const std::vector< JsonObject >& devices ) {
    auto responseBuilder = ResponseBuildersFactory::createGetDevicesResponseBuilder( rpcMessage );
    if( !responseBuilder )
        return;

    auto devicesList = devices;
    auto params = rpcMessage.getParams();
    if( !params.empty() && json_contains( params, "filter" ) ) {
        bool bypassAllowed;
        if( json_get( params[ "filter" ], "bypassAllowed", bypassAllowed ) ) {
            if( bypassAllowed ) {
                filterBypassDevices( devicesList );
            }
        }
    }

    /* apply transformations specific to the Hub API version */
    for( const auto& d : devicesList ) {
        DeviceAdditionalInfoArray additionalInfo;
        DeviceFirmwareInfoArray firmwareInfo;
        ArmedState armed;
        Json houseModesOptions;
        std::tie( additionalInfo, firmwareInfo, armed, houseModesOptions ) = prepareDeviceResponseFields( d );

        responseBuilder->addDevice( json_get_str( d, "_id" ),
                                    json_get_str( d, "deviceTypeId" ),
                                    json_get_str( d, "gatewayId" ),
                                    json_get_str( d, "name" ),
                                    json_get_str( d, "type" ),
                                    json_get_str( d, "roomId" ),
                                    json_get_str( d, "category" ),
                                    json_get_str( d, "subcategory" ),
                                    json_get_str( d, "parentDeviceId" ),
                                    json_get_str( d, "security" ),
                                    json_get_str( d, "status" ),
                                    additionalInfo,
                                    firmwareInfo,
                                    json_get_bool( d, "batteryPowered" ),
                                    json_get_bool( d, "reachable" ),
                                    json_get_bool( d, "ready" ),
                                    armed,
                                    json_get_bool( d, "persistent" ),
                                    json_get_bool( d, "parentRoom", CoreD::Model::Resources::Rooms::ParentRoomDefault ),
                                    json_get_str( d, "protectConfig" ) );
    }
    JsonObject& joAnswer = rpcMessage.getAnswer();
    auto response = responseBuilder->buildResponse();
    joAnswer[ "result" ] = std::move( response[ "result" ] );
    joAnswer[ "api" ] = std::move( response[ "api" ] );
}

void Proto::buildDataListDevicesResponse( rpc::Message& rpcMessage,
                                          const std::vector< JsonObject >& devices,
                                          FilterDataListI* filter ) {
    ResponseBuilder::DevicesListBuilder::buildDataListDevicesResponse( rpcMessage, devices, filter );
}

bool Proto::redirectCustomServiceMessage( rpc::Message& rpcMessage ) {
    LOGGER_TRACE_RAW( cc::trace( "Checking if a custom service message should be redirected" ) );

    auto& joMessage = rpcMessage.getMessage();
    const auto method = rpcMessage.getMethod();

    /*            prefix.domain.method
     *  method == hub.vidoo.kyky */
    const auto domainPos = method.find( '.' );
    if( domainPos == std::string::npos ) {
        return false;
    }
    const auto prefix = method.substr( 0, domainPos );
    if( prefix != "hub" ) {
        return false;
    }
    const auto methodPos = method.find( '.', domainPos + 1 );
    if( methodPos == std::string::npos ) {
        return false;
    }
    const auto requestDomain = method.substr( domainPos + 1, methodPos - domainPos - 1 );

    // TODO: implement multicast message redirect
    const auto* customService = m_uiService.matchCustomService( requestDomain );
    if( !customService ) {
        return false;
    }

    LOGGER_TRACE_RAW( cc::trace( "Redirect a custom service message: " ) + cc::j( joMessage ) );

    rpcMessage.answerPrepare();
    rpcMessage.copySender();
    auto& joAnswer = rpcMessage.getAnswer();

    if( joMessage.count( "api" ) != 0 ) {
        joAnswer[ "api" ] = std::move( joMessage[ "api" ] );
    }

    m_uiService.sendAsyncRequest( customService->m_spreadServiceName,
                                  joMessage,
                                  CUSTOM_SERVICE_MESSAGE,
                                  [joAnswer, this]( const JsonObject& response ) mutable {
                                      setErrorIfHas( joAnswer, response );
                                      joAnswer[ "result" ] = std::move( response );
                                      json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                                      m_uiService.getNmaClient()->postMessage( joAnswer );
                                  },
                                  [joAnswer, method, this]() mutable {
                                      Log::error( cc::error( "Received command not handled: " ) + cc::note( method ) );
                                      setError( joAnswer, Errors::UNKNOWN_METHOD );
                                      m_uiService.getNmaClient()->postMessage( joAnswer );
                                  },
                                  customService->m_requestTimeout );

    joAnswer = "";

    return true;
}

using OptionalFieldsUPtr = std::unique_ptr< Uid::ResponseBuilder::GetItems::IResponseBuilder::OptionalFields >;

void Proto::buildGetItemsResponse( rpc::Message& rpcMessage, const std::vector< JsonObject >& items ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );
    ResponseBuilder::ItemsListBuilder::buildGetItemsResponse( rpcMessage, items, Service::getInstance() );
}

void Proto::buildDataListItemsResponse( rpc::Message& rpcMessage,
                                        const std::vector< JsonObject >& items,
                                        FilterDataListI* filter ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );

    auto responseBuilder = ResponseBuildersFactory::createDataListItemsResponseBuilder(
        rpcMessage, coreDInterface.getConfiguration(), filter );
    if( !responseBuilder )
        return;

    try {
        for( const auto& i : items ) {
            using namespace CoreD::Model;
            ValueUPtr value;
            OptionalFieldsUPtr optionalFieldsPtr;
            std::tie( value, optionalFieldsPtr ) = ResponseBuilder::ItemsListBuilder::prepareItemFields( i );
            auto& optionalFields = *optionalFieldsPtr.get();

            responseBuilder->addItem( json_get_str( i, "_id" ),
                                      json_get_str( i, "deviceId" ),
                                      json_get_str( i, "name" ),
                                      json_get_bool( i, "hasGetter" ),
                                      json_get_bool( i, "hasSetter" ),
                                      json_get_bool( i, "show" ),
                                      *value,
                                      optionalFields );
        }

        JsonObject& joAnswer = rpcMessage.getAnswer();
        auto response = responseBuilder->buildResponse();
        joAnswer[ "result" ][ "items" ] = std::move( response[ "result" ][ "items" ] );
        joAnswer[ "api" ] = std::move( response[ "api" ] );
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        Log::error( cc::error( "buildGetItemsResponse: CoreDModelError occurred - " ) + error.what() );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
    catch( ... ) {
        Log::error( cc::error( "buildGetItemsResponse: unknown error occurred - " ) );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
}

void Proto::buildGetDeviceSettingsResponse(
    rpc::Message& rpcMessage,
    const CoreDRemoteInterface::CoreDRemoteInterface::DeviceSettings& deviceSettings ) {
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( Service::getInstance() );

        auto responseBuilder = ResponseBuildersFactory::createGetDeviceSettingResponseBuilder(
            rpcMessage, coreDInterface.getConfiguration() );
        if( !responseBuilder ) {
            LOGGER_DEBUG( "there is no response builder" );
            return;
        }

        HubObjectListDeviceFilter deviceFilter( rpcMessage );

        for( const auto& pair : deviceSettings ) {
            const auto& setting = pair.second;

            if( !deviceFilter.filterDeviceIds( setting.getDeviceId() ) )
                continue;

            responseBuilder->addDeviceSetting( pair.first,
                                               setting.getDeviceId(),
                                               setting.getLabel(),
                                               setting.getDescription(),
                                               setting.getValueType(),
                                               setting.getValue(),
                                               setting.getDefaultValue(),
                                               setting.getLowerLimitValue(),
                                               setting.getUpperLimitValue(),
                                               setting.getValueStep(),
                                               setting.getEnum(),
                                               setting.getInfo(),
                                               setting.getElementsMaxNumber(),
                                               setting.getSynchronizationStatus(),
                                               setting.getHasSetterValue(),
                                               setting.getTargetChannelSupportedValue() );
        }

        JsonObject& joAnswer = rpcMessage.getAnswer();
        auto response = responseBuilder->buildResponse();
        joAnswer[ "result" ] = std::move( response[ "result" ] );
        joAnswer[ "api" ] = std::move( response[ "api" ] );
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "error occurred - " ) + e.what() );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
}

void Proto::getDevices( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    buildGetDevicesResponse( rpcMessage, coreDInterface.getDevices() );
}

void Proto::getItems( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    buildGetItemsResponse( rpcMessage, coreDInterface.getItems() );
}

void Proto::getDeviceSetting( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    JsonArray filterIds;

    auto& params = rpcMessage.getParams();
    auto idsIt = params.find( "ids" );
    if( idsIt != params.end() ) {
        try {
            jsonCheckArray( params, "ids", []( const JsonArray& arr ) {
                for( const auto& item : arr )
                    jsonCheckString( item.get< std::string >(), JsonStringChecks::NON_EMPTY );
            } );
        }
        catch( const std::runtime_error& ex ) {
            Log::error( cc::error( "Check of an 'ids' array is failed: " ) + cc::error( ex.what() ) );
            Json error;
            Errors::getWrongParams( "ids", error );
            setErrorObject( rpcMessage.getAnswer(), error );
            return;
        }
        if( idsIt->empty() ) {
            Json error;
            Errors::getEmptyString( "ids", error );
            setErrorObject( rpcMessage.getAnswer(), error );
            return;
        }

        Json dbQuery = JSON( CoreD::Model::Resources::DeviceSettingFields::Id << JSON( "$in" << *idsIt ) );
        auto result = coreDInterface.getDeviceSettingsBy( dbQuery );
        buildGetDeviceSettingsResponse( rpcMessage, result );
        return;
    }
    buildGetDeviceSettingsResponse( rpcMessage, coreDInterface.getDeviceSettings() );
}

void Proto::setDeviceSettingValue( rpc::Message& rpcMessage ) {
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( m_uiService );
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = coreDInterface.setSettingValue( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const CoreD::DeviceBusy& e ) {
        setError( rpcMessage.getAnswer(), Errors::DEVICE_BUSY, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::setDeviceSettingDictionaryValue( rpc::Message& rpcMessage ) {
    using namespace CoreD::Model;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( m_uiService );
        auto& joAnswer = rpcMessage.getAnswer();
        auto response = coreDInterface.setSettingDictionaryValue( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const CoreD::DeviceBusy& e ) {
        setError( rpcMessage.getAnswer(), Errors::DEVICE_BUSY, e.what() );
    }
    catch( const CoreD::Model::StringIsTooLongError& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::resetDeviceSetting( rpc::Message& rpcMessage ) {
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( m_uiService );
        auto& joAnswer = rpcMessage.getAnswer();
        auto response = coreDInterface.resetSetting( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const CoreD::DeviceBusy& e ) {
        setError( rpcMessage.getAnswer(), Errors::DEVICE_BUSY, e.what() );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::checkDevice( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( m_uiService );
    auto& joAnswer = rpcMessage.getAnswer();

    auto response = coreDInterface.checkDevice( rpcMessage.getParams() );
    setErrorIfHas( joAnswer, response );
    joAnswer[ "result" ] = std::move( response );
    json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
}

void Proto::handleSshTunnelSupportEnabledSet( rpc::Message& rpcMessage ) {
    try {
        const JsonObject& joParams = rpcMessage.getParams();
        auto& error = rpcMessage.getAnswer()[ "error" ];

        bool enabled = false;
        std::string time;  // Example: 2020-12-14T18:00:00
        int endTime = 0;

        if( !json_get( joParams, "enabled", enabled ) ) {
            Errors::getWrongParams( "enabled", error );
            return;
        }

        if( enabled && json_get( joParams, "endTime", time ) ) {
            std::tm tm = {};
            if( !strptime( time.c_str(), "%Y-%m-%dT%T", &tm ) ) {
                Errors::getWrongParams( "endTime", error );
                return;
            }

            endTime = mktime( &tm );

            if( endTime <= 0 ) {
                Errors::getWrongParams( "endTime", error );
                return;
            }

            auto now = std::chrono::duration_cast< std::chrono::seconds >(
                std::chrono::system_clock::now().time_since_epoch() );
            if( std::chrono::seconds( endTime ) <= now ) {
                Errors::getInvalidValueRange( "endTime", error, "endTime already expired" );
                return;
            }
        }

        Network::RemoteInterface interface{ m_uiService };
        interface.sshTunnelSupportEnabledSet( enabled, endTime );
    }
    catch( const std::exception& err ) {
        Log::error( cc::error( "Exception during handleSshTunnelSupportEnabledSet" ) + err.what() );
    }
}

namespace {
JsonObject accessKeysInfo( Services::Service& service ) {
    auto currentCredentialsVersion = HubOfflineCommandsHandler::getCurrentCredentialsVersion( service.getDBClient() );
    auto currentCredentials = HubOfflineCommandsHandler::getCurrentCredentials( service.getDBClient() );

    bool hasCreds = currentCredentials.size() > 0;

    return { { "version", currentCredentialsVersion }, { "hasCredentials", hasCreds } };
}

}  // namespace

void Proto::getHubInfo( rpc::Message& rpcMessage ) {
    auto result = Common::getHubInfo( m_uiService );
    {
        JsonObject accessKeys = accessKeysInfo( m_uiService );
        result[ "localMode" ] = {
            { "accessKeys", accessKeys },
        };
    }
    rpcMessage.getAnswer()[ "result" ] = result;
}

void Proto::setServiceNotifications( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    JsonArray ids;
    if( !getFieldAsArray( joParams, "ids", joAnswer, ids ) ) {
        LOGGER_DEBUG( "no ids" );
        return;
    }
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    JsonObject result = coreDInterface.setServiceNotifications( ids );
    if( hasError( result ) ) {
        joAnswer[ "error" ] = result[ "error" ];
        return;
    }
    joAnswer[ "result" ] = result[ "result" ];
}

void Proto::handleSceneList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }
    auto result = getScenesManager().checkScenesFilterParams( params );
    if( !result.empty() ) {
        answer[ "error" ] = result[ "error" ];
    }
    else {
        auto list = getScenesManager().getScenesList( params );
        answer[ "result" ][ "scenes" ] = list;
    }
}

void Proto::handleGlobalSceneList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    if( !Acl::check( params, Resource::SCENES, Permission::VIEW, answer ) ) {
        return;
    }
    bool onlyId = json_get_bool( params, "onlyId" );
    answer[ "result" ][ "scenes" ] = getScenesManager().getGlobalScenesList( onlyId );
}

void Proto::handleRunScene( rpc::Message& rpcMessage ) {
    handleRunScene( rpcMessage, /*isGlobal*/ false );
}

void Proto::handleStopScene( rpc::Message& rpcMessage ) {
    handleStopScene( rpcMessage, /*isGlobal*/ false );
}

void Proto::handleHubReboot( rpc::Message& rpcMessage ) {
    using PostponedTask = HubPlatform::Services::PostponedTask;

    JsonObject& joAnswer = rpcMessage.getAnswer();
    int intervalSeconds = 5;

    Log::info( cc::info( "hub.reboot: reboot starts in " + std::to_string( intervalSeconds ) + " sec..." ) );

    auto environmentPtr = m_uiService.getEnvironment();
    auto doRebootFunc = [environmentPtr]( PostponedTask& ) {
        if( environmentPtr ) {
            Log::trace( cc::trace( "hub.reboot: reboot started" ) );
            environmentPtr->reboot();
        }
        else {
            Log::warn( cc::warn( "Environment is not initialized. Reboot was skipped." ) );
        }
    };

    auto interval = std::chrono::seconds( intervalSeconds );
    m_uiService.createPostponedTask( doRebootFunc )->start( interval, false );
}

void Proto::handleHubPoweroff( rpc::Message& rpcMessage ) {
    /* theoretically a 'poweroff' functionality can be provided for all hubs,
     * but now allow it only for h2_secure hubs */
#if defined( __HUB_TYPE_H2_SECURE__ )
    HardwareManager::RemoteInterface interface( getService() );
    interface.hubPowerOff();
#else
    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
#endif
}

void Proto::handleDataList( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );

    // Send only those collections, whose corresponding keys passed with the request

    auto& messageParams = rpcMessage.getParams();

    if( messageParams == JSON_EMPTY_OBJ || messageParams.empty() ) {
        ::setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, "No collection names passed" );
        return;
    }

    struct Request {
        std::string name;
        JsonObject params;
    };

    size_t keysPassed = 0;
    std::vector< std::pair< std::string, std::string > > collections = {
        { "devices", "ids" }, { "items", "ids" },      { "rooms", "ids" },
        { "scenes", "ids" },  { "features", "names" }, { "settings", "names" }
    };
    std::vector< Request > requests( collections.size() );
    for( auto index = 0; index < collections.size(); index++ ) {
        const std::string& name = collections[ index ].first;
        const std::string& idsFieldName = collections[ index ].second;

        assert( collections.size() == requests.size() );
        Request& req = requests[ index ];
        req.name = name;

        bool hasCollectionKey = rpcMessage.getParams().count( name ) > 0;
        bool fetchedObject = hasCollectionKey && json_get_obj( messageParams, name, req.params );
        if( hasCollectionKey || fetchedObject ) {
            keysPassed++;

            DataListFilterUPtr dataFilterPtr;

            try {
                dataFilterPtr = initDataListFilter( req.params, idsFieldName );
            }
            catch( const std::runtime_error& err ) {
                auto& error = rpcMessage.getAnswer()[ "error" ];
                Errors::getWrongParams( name, error );
                return;
            }

            FilterDataListI& dataFilter = *dataFilterPtr;
            if( name == "devices" ) {
                buildDataListDevicesResponse( rpcMessage, coreDInterface.getDevices(), dataFilterPtr.get() );
            }
            else if( name == "items" ) {
                buildDataListItemsResponse( rpcMessage, coreDInterface.getItems(), dataFilterPtr.get() );
            }
            else if( name == "rooms" ) {
                auto records = coreDInterface.getAllRooms();
                buildDataListCollectionResponse( rpcMessage, records, name, "_id", dataFilterPtr.get() );
            }
            else if( name == "scenes" ) {
                auto records = this->m_scenesRemoteInterface.getScenesList( messageParams[ "scenes" ] );
                buildDataListCollectionResponse( rpcMessage, records, name, "_id", dataFilterPtr.get() );
            }
            else if( name == "features" ) {
                HubFeaturesCommandsHandler featuresHandler( m_uiService );
                buildDataListCollectionResponse(
                    rpcMessage, featuresHandler.getFeaturesList(), name, dataFilterPtr.get() );
            }
            else if( name == "settings" ) {
                HubSettingsCommandsHandler settingsHandler( m_uiService );
                buildDataListCollectionResponse(
                    rpcMessage, settingsHandler.getSettingsList(), name, "name", dataFilterPtr.get() );
            }
        }
    }

    if( 0 == keysPassed ) {
        ::setError( rpcMessage.getAnswer(), Errors::WRONG_PARAMS, "No collection names passed" );
        return;
    }
}

void Proto::handleRunGlobalScene( rpc::Message& rpcMessage ) {
    handleRunScene( rpcMessage, /*isGlobal*/ true );
}

void Proto::handleRunScene( rpc::Message& rpcMessage, bool isGlobal ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( isGlobal ) {
        if( !getField( params, "_id", answer, id ) ) {
            return;
        }
    }
    else {
        if( !getField( params, "sceneId", answer, id ) ) {
            return;
        }
    }
    if( !Acl::check( params, Resource::SCENES, Permission::EXECUTE, answer ) ) {
        return;
    }
    std::string blockType, groupId;
    auto result = getOptionalField( params, "block", blockType ) ?
                      ( getOptionalField( params, "group", groupId ) ?
                            getScenesManager().runScene( id, isGlobal, blockType, groupId ) :
                            getScenesManager().runScene( id, isGlobal, blockType ) ) :
                      getScenesManager().runScene( id, isGlobal );
    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void Proto::handleStopScene( rpc::Message& rpcMessage, bool isGlobal ) {
    const auto& params = rpcMessage.getParams();
    auto& answer = rpcMessage.getAnswer();
    std::string id;
    if( isGlobal ) {
        if( !getField( params, "_id", answer, id ) ) {
            return;
        }
    }
    else {
        const auto SceneId = "sceneId";
        if( !getField( params, SceneId, answer, id ) ) {
            auto& error = rpcMessage.getAnswer()[ "error" ];
            Errors::getBadRequest( SceneId, error );
            return;
        }
        if( id.empty() ) {
            auto& error = rpcMessage.getAnswer()[ "error" ];
            Errors::getEmptyString( SceneId, error );
            return;
        }
    }
    if( !Acl::check( params, Resource::SCENES, Permission::EXECUTE, answer ) ) {
        return;
    }

    std::string blockType;
    const auto Block = "block";
    if( getOptionalField( params, Block, blockType ) ) {
        if( id.empty() ) {
            auto& error = rpcMessage.getAnswer()[ "error" ];
            Errors::getEmptyString( Block, error );
        }
    }

    const auto result = getScenesManager().stopScene( id, isGlobal, blockType );

    if( hasError( result ) ) {
        answer[ "error" ] = result[ "error" ];
        return;
    }
    answer[ "result" ] = result;
}

void Proto::buildDataListCollectionResponse( rpc::Message& rpcMessage,
                                             const std::vector< JsonObject >& records,
                                             const std::string& collectionName,
                                             const std::string& idFieldName,
                                             FilterDataListI* filter ) {
    JsonObject result = JSON_EMPTY_OBJ;

    std::set< std::string > allIds;
    for( const auto& i : records ) {
        const auto& id = json_get_str( i, idFieldName );
        for( auto fieldIt = i.begin(); fieldIt != i.end(); fieldIt++ ) {
            auto key = fieldIt.key();
            if( key == idFieldName ) {
                continue;
            }
            else {
                const auto& value = fieldIt.value();

                if( nullptr == filter || filter->emptyIds() || filter->hasId( id ) ) {
                    if( nullptr == filter || filter->hasIncludeField( key ) )
                        result[ id ][ key ] = value;
                }
            }
        }

        allIds.insert( id );
    }

    std::vector< std::string > notInSet;
    if( filter && filter->idsNotInSet( allIds, notInSet ) ) {
        std::for_each( begin( notInSet ), end( notInSet ), [&result]( const std::string& id ) {
            result[ id ] = JsonObject( nullptr );
        } );
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer[ "result" ][ collectionName ] = std::move( result );
    //        joAnswer[ "api" ] = std::move( response[ "api" ] );
}

void Proto::buildDataListCollectionResponse( rpc::Message& rpcMessage,
                                             const JsonObject& records,
                                             const std::string& collectionName,
                                             FilterDataListI* filter ) {
    JsonObject result = JSON_EMPTY_OBJ;

    std::set< std::string > allIds;
    for( auto recordIt = records.begin(); recordIt != records.end(); recordIt++ ) {
        auto id = recordIt.key();
        for( auto fieldIt = recordIt.value().begin(); fieldIt != recordIt.value().end(); fieldIt++ ) {
            auto key = fieldIt.key();
            const auto& value = fieldIt.value();

            if( nullptr == filter || filter->emptyIds() || filter->hasId( id ) ) {
                if( nullptr == filter || filter->hasIncludeField( key ) )
                    result[ id ][ key ] = value;
            }
        }

        allIds.insert( id );
    }

    std::vector< std::string > notInSet;
    if( filter && filter->idsNotInSet( allIds, notInSet ) ) {
        std::for_each( begin( notInSet ), end( notInSet ), [&result]( const std::string& id ) {
            result[ id ] = JsonObject( nullptr );
        } );
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer[ "result" ][ collectionName ] = std::move( result );
    //        joAnswer[ "api" ] = std::move( response[ "api" ] );
}

std::string Proto::getRegisterMethod() {
    Network::RemoteInterface networkInterface{ getService() };
    CoreDRemoteInterface::CoreDRemoteInterface interface{ getService() };
    const auto geopositionPair = interface.getGeoPosition();

    JsonObject message = prepareMethod( "register" );
    JsonObject joParams = JSON_EMPTY_OBJ;
    json_add_str( joParams, "serial", Ezlo::serial() );
    json_add_str( joParams, "controller_uuid", Ezlo::uuid() );
    json_add_str( joParams, "firmware", FirmwareInfoHelper::getFirmwareVersionString() );
    json_add_int( joParams, "timeOffset", Ezlo::getTimeOffset() );

    json_add_str( joParams, "hubType", Ezlo::hubType() );
    json_add_str( joParams, "location", interface.getHubLocationState( geopositionPair ) );

    json_add_str( joParams, "mac_address", Ezlo::macAddress() );

    size_t rxBufferSize = this->m_uiService.getEnvironment()->getNmaClientRxBufferSize();
    json_add_int( joParams, "maxFrameSize", rxBufferSize );

    auto activeInterface = networkInterface.getActiveNetworkInterface();
    if( !activeInterface.empty() ) {
        auto media = json_get_str( activeInterface, "type" );
        json_add_str( joParams, "media", media );
        if( media == "modem" )
            json_add_str( joParams, "connection", "limited" );
    }
    else {
        LOGGER_WARN_RAW( cc::warn( "No active interface detected" ) );
    }

    message[ "params" ] = std::move( joParams );
    return json_write( message );
}

size_t Proto::getNmaPingInterval() {
    Network::RemoteInterface networkInterface{ getService() };
    auto activeInterface = networkInterface.getActiveNetworkInterface();
    if( !activeInterface.empty() ) {
        auto media = json_get_str( activeInterface, "type" );
        if( media == "modem" )
            return 300;
    }
    return 60;
}

void Proto::handleCloudControllerResponse( rpc::Message& rpcMessage ) {
    m_uiService.getIccManager().handleCloudControllerResponse( rpcMessage );
}

void Proto::handleBridgeControllerConnectionSet( rpc::Message& rpcMessage ) {
    m_uiService.getClusterManager().handleBridgeControllerConnectionSet( rpcMessage );
}

void Proto::handleBridgeConnectionsList( rpc::Message& rpcMessage ) {
    m_uiService.getClusterManager().handleBridgeConnectionsList( rpcMessage );
}

void Proto::handleBridgeControllersList( rpc::Message& rpcMessage ) {
    m_uiService.getClusterManager().handleBridgeControllersList( rpcMessage );
}

void Proto::handleHubOfflineAnonymousAccessEnabledSet( rpc::Message& rpcMessage ) {
    HubOfflineCommandsHandler( getService() ).handleOfflineAnonymousAccessEnabledSetRequest( rpcMessage );
}

void Proto::handleHubOfflineInsecureAccessEnabledSet( rpc::Message& rpcMessage ) {
    HubOfflineCommandsHandler( getService() ).handleOfflineInsecureAccessEnabledSetRequest( rpcMessage );
}

void Proto::handleHubOfflineCredentialsUpdated( rpc::Message& rpcMessage ) {
    HubOfflineCommandsHandler( getService() ).handleCredentialsUpdatedRequest( rpcMessage );
}

void Proto::handleHubCloudRegisterRepeat( rpc::Message& rpcMessage ) {
    auto& answer = rpcMessage.getAnswer();
    answer[ "result" ] = JSON_EMPTY_OBJ;
    m_uiService.addTask( [this]() { m_uiService.getNmaClient()->onHubRegisterRepeat(); } );
}

void Proto::handleHubFeaturesList( rpc::Message& rpcMessage ) {
    HubFeaturesCommandsHandler( getService() ).handleHubFeaturesList( rpcMessage );
}

void Proto::handleHubFeatureStatusSet( rpc::Message& rpcMessage ) {
    HubFeaturesCommandsHandler( getService() ).handleHubFeatureStatusSet( rpcMessage );
}

void Proto::handleHubSettingsList( rpc::Message& rpcMessage ) {
    HubSettingsCommandsHandler( getService() ).handleHubSettingsList( rpcMessage );
}

void Proto::handleSettingValueSet( rpc::Message& rpcMessage ) {
    HubSettingsCommandsHandler( getService() ).handleSettingValueSet( rpcMessage );
}

void Proto::handleHubRoomList( rpc::Message& rpcMessage ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto response = coreDInterface.getAllRooms();

    auto& answer = rpcMessage.getAnswer();
    answer[ "result" ] = std::move( response );
}

void Proto::updateDeviceFirmware( rpc::Message& rpcMessage ) {
    using namespace HubPlatform;
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    rpcMessage.getResult() = remoteInterface.updateDeviceFirmware( rpcMessage.getParams() );
}

bool Proto::invokeHandler( const std::string& method, rpc::Message& message ) {
    using RequestHandler = void ( Proto::* )( rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap handlers = {
        { "Ping", &Proto::ping },
        { "getWifiList", &Proto::getWifiList },
        { "connectWifi", &Proto::connectWifi },
        { "deleteWifi", &Proto::deleteWifi },
        { "getStatusWifi", &Proto::getStatusWifi },
        { "resetWifi", &Proto::resetWifi },
        { "getEzloIp", &Proto::getEzloIp },
        { "getEzloMac", &Proto::getEzloMac },
        { "hub.version.get", &Proto::getEzloVersion },
        { "hub.description.set", &Proto::setDescription },
        { "setEzloLogLevel", &Proto::setEzloLogLevel },
        { "hub.log.local.set", &Proto::setEzloLogSettings },
        { "hub.log.send", &Proto::hubLogSend },
        { "hub.server.request", &Proto::requestNewNMAServer },
        { "hub.service.info.get", &Proto::getServiceInfo },
        { "hub.devices.list", &Proto::getDevices },
        { "hub.devices.service.notifications.set", &Proto::setServiceNotifications },
        { "hub.items.list", &Proto::getItems },
        { "hub.item.value.set", &Proto::setItemValue },
        { "hub.item.value.percent.apply", &Proto::changeItemValueLevelByPercent },
        { "hub.device.settings.list", &Proto::getDeviceSetting },
        { "hub.device.setting.value.set", &Proto::setDeviceSettingValue },
        { "hub.device.setting.dictionary.value.set", &Proto::setDeviceSettingDictionaryValue },
        { "hub.device.setting.reset", &Proto::resetDeviceSetting },
        { "hub.device.check", &Proto::checkDevice },
        { "hub.info.get", &Proto::getHubInfo },
        { "hub.scenes.list", &Proto::handleSceneList },
        { "hub.scenes.run", &Proto::handleRunScene },
        { "hub.scenes.stop", &Proto::handleStopScene },
        { "hub.scenes.global.list", &Proto::handleGlobalSceneList },
        { "hub.scenes.global.run", &Proto::handleRunGlobalScene },
        { "hub.scenes.expressions.set", &Proto::handleSceneExpressionsSet },
        { "hub.scenes.expressions.delete", &Proto::handleSceneExpressionsDelete },
        { "hub.scenes.expressions.list", &Proto::handleSceneExpressionsList },
        { "hub.reboot", &Proto::handleHubReboot },
        { "hub.poweroff", &Proto::handleHubPoweroff },
        { "hub.data.list", &Proto::handleDataList },
        { "cloud.controller_raw_command", &Proto::handleCloudControllerResponse },
        { "hub.bridge.controller.connection.set", &Proto::handleBridgeControllerConnectionSet },
        { "hub.bridge.connections.list", &Proto::handleBridgeConnectionsList },
        { "hub.bridge.controllers.list", &Proto::handleBridgeControllersList },
        { "hub.offline.anonymous_access.enabled.set", &Proto::handleHubOfflineAnonymousAccessEnabledSet },
        { "hub.offline.insecure_access.enabled.set", &Proto::handleHubOfflineInsecureAccessEnabledSet },
        { "hub.offline.credentials.updated", &Proto::handleHubOfflineCredentialsUpdated },
        { "hub.cloud.register.repeat", &Proto::handleHubCloudRegisterRepeat },
        { "hub.features.list", &Proto::handleHubFeaturesList },
        { "hub.feature.status.set", &Proto::handleHubFeatureStatusSet },
        { "hub.settings.list", &Proto::handleHubSettingsList },
        { "hub.setting.value.set", &Proto::handleSettingValueSet },
        { "hub.ssh_tunnel.support.enabled.set", &Proto::handleSshTunnelSupportEnabledSet },
        { "hub.room.list", &Proto::handleHubRoomList },
        { "hub.cloud.subscription.create", &Proto::handleCloudSubscriptionCreate },
        { "hub.cloud.subscription.update", &Proto::handleCloudSubscriptionUpdate },
        { "hub.cloud.subscription.delete", &Proto::handleCloudSubscriptionDelete },
        { "hub.cloud.subscriptions.list", &Proto::handleCloudSubscriptionsList },
        { "hub.cloud.subscription.dispatch", &Proto::handleCloudSubscriptionDispatch },
        { "hub.videoforward", &Proto::handleHubVideoForward },
        { "hub.vidoo.elements.event", &Proto::handleHubVidooElementsEvent },
        { "hub.device.group.create", &Proto::handleDeviceGroupCreate },
        { "hub.device.group.delete", &Proto::handleDeviceGroupDelete },
        { "hub.device.group.update", &Proto::handleDeviceGroupUpdate },
        { "hub.device.groups.list", &Proto::handleDeviceGroupsList },
        { "hub.device.group.get", &Proto::handleDeviceGroupGet },
        { "hub.device.group.device.add", &Proto::handleDeviceGroupDeviceAdd },
        { "hub.device.group.device.remove", &Proto::handleDeviceGroupDeviceRemove },
        { "hub.item.group.create", &Proto::handleItemGroupCreate },
        { "hub.item.group.delete", &Proto::handleItemGroupDelete },
        { "hub.item.group.update", &Proto::handleItemGroupUpdate },
        { "hub.item.groups.list", &Proto::handleItemGroupsList },
        { "hub.item.group.get", &Proto::handleItemGroupsGet },
        { "hub.device.item.group.expand", &Proto::handleDeviceItemGroupExpand },
        { "hub.device.group.find", &Proto::handleDeviceGroupsFind },
        { "hub.policy.create", &Proto::handlePolicyCreate },
        { "hub.policy.delete", &Proto::handlePolicyDelete },
        { "hub.policy.update", &Proto::handlePolicyUpdate },
        { "hub.policy.get", &Proto::handlePolicyGet },
        { "hub.policies.list", &Proto::handlePolicyList },
        { "hub.protect.signal.list", &Proto::handleContactIdsList },
        { "hub.protect.security.configs.list", &Proto::handleSecurityConfigList },
        { "hub.offline.cloud.data.update", &Proto::handleCloudLocalDataUpdate },
        { "hub.notifications.smtp.settings.set", &Proto::handleNotificationsSmtpSettingsSet },
        { "hub.notifications.smtp.settings.get", &Proto::handleNotificationsSmtpSettingsGet },
        { "hub.notifications.smtp.send", &Proto::handleNotificationsSmtpSend },
        { "hub.offline.cloud.data.info.get", &Proto::handleHubOfflineCloudDataInfoGet },
    };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( message );
        return true;
    }
    else {
        return false;
    }
}

bool Proto::invokeExtendedHandler( const std::string& method, const std::string& submethod, rpc::Message& message ) {
    using RequestHandler = bool ( Proto::* )( const std::string&, rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap handlers = {
        { "hub", &Proto::invokeHubHandler },
    };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        Log::info( cc::info( "Process COMMON command: " ) + cc::note( method ) );
        return ( this->*handler )( submethod, message );
    }

    return false;
}

Service& Proto::getService() {
    return m_uiService;
}

void Proto::handleSceneExpressionsSet( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    const std::string expressionName = json_get_str( params, "name" );
    Json expressionParams = JSON_EMPTY_OBJ;
    if( !json_get_obj( params, "params", expressionParams ) ) {
        // params is empty
    }
    Json expressionValue = JSON_EMPTY_OBJ;
    if( !json_get( params, "value", expressionValue ) ) {
    }

    std::string code = json_get_str( params, "code" );
    std::string valueType = json_get_str( params, "valueType" );
    Json metadata;
    json_get_obj( params, "metadata", metadata );
    bool variable = json_get_bool( params, "variable" );
    Json value;

    json_get( params, "value", value );

    auto response = m_scenesRemoteInterface.upsertSceneExpression(
        expressionName, expressionParams, code, valueType, metadata, variable, value );
    Json& joAnswer = rpcMessage.getAnswer();
    setErrorIfHas( joAnswer, response );
}

void Proto::handleSceneExpressionsDelete( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    const std::string expressionName = json_get_str( params, "name" );

    auto response = m_scenesRemoteInterface.deleteSceneExpression( expressionName );
    Json& joAnswer = rpcMessage.getAnswer();
    setErrorIfHas( joAnswer, response );
}

void Proto::handleSceneExpressionsList( rpc::Message& rpcMessage ) {
    const auto& params = rpcMessage.getParams();
    bool showCode = json_get_bool( params, "showCode", false );
    bool strict = json_get_bool( params, "strict", false );
    JsonArray filterTypes;
    json_get_arr( params, "filterTypes", filterTypes );
    std::string filterType = json_get_str( params, "filterType" );

    std::vector< std::string > filters;

    for( const auto& filter : filterTypes ) {
        filters.push_back( filter );
    }

    if( !filterType.empty() ) {
        filters.push_back( filterType );
    }

    bool variable;

    Json variableRef;
    bool hasVariable = json_get( params, "variable", variableRef );
    variable = json_get_bool( params, "variable" );

    JsonArray names;
    json_get_arr( params, "names", names );

    bool showValue = json_get_bool( params, "showValue", true );

    auto response = m_scenesRemoteInterface.listSceneExpressions(
        showCode, strict, filters, hasVariable, variable, names, showValue );
    // TODO: meshbot: format expression here
    Json& answer = rpcMessage.getAnswer();
    setErrorIfHas( answer, response );
    answer[ "result" ] = std::move( response );
}

void Proto::handleCloudSubscriptionCreate( rpc::Message& rpcMessage ) {
    HubCloudSubscriptionsHandler( m_uiService ).handleCloudSubscriptionCreate( rpcMessage );
}

void Proto::handleCloudSubscriptionUpdate( rpc::Message& rpcMessage ) {
    HubCloudSubscriptionsHandler( m_uiService ).handleCloudSubscriptionUpdate( rpcMessage );
}

void Proto::handleCloudSubscriptionDelete( rpc::Message& rpcMessage ) {
    HubCloudSubscriptionsHandler( m_uiService ).handleCloudSubscriptionDelete( rpcMessage );
}

void Proto::handleCloudSubscriptionsList( rpc::Message& rpcMessage ) {
    HubCloudSubscriptionsHandler( m_uiService ).handleCloudSubscriptionsList( rpcMessage );
}

void Proto::handleCloudSubscriptionDispatch( rpc::Message& rpcMessage ) {
    HubCloudSubscriptionsHandler( m_uiService ).handleCloudSubscriptionDispatch( rpcMessage );
}

void Proto::handleHubVideoForward( rpc::Message& rpcMessage ) {
    HubVideoForwardHandler( m_uiService ).handleVideoForward( rpcMessage );
}

void Proto::handleHubVidooElementsEvent( rpc::Message& rpcMessage ) {
    HubVidooCommandsHandler( m_uiService ).handleHubVidooElementsEvent( rpcMessage );
}

void fillAnswer( JsonObject& answer, const JsonObject& result ) {
    if( !setErrorIfHas( answer, result ) ) {
        answer[ "result" ] = result;
    }
    else {
        answer[ "result" ] = JSON_EMPTY_OBJ;
    }
}

void Proto::handleDeviceGroupCreate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().create( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupDelete( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().deleteGroup( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupUpdate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().update( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupsList( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().list( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupGet( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().get( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupDeviceAdd( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().addDevices( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupDeviceRemove( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().removeDevices( params );
    fillAnswer( answer, result );
}

void Proto::handleItemGroupCreate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->itemGroup().create( params );
    fillAnswer( answer, result );
}

void Proto::handleItemGroupDelete( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->itemGroup().deleteGroup( params );
    fillAnswer( answer, result );
}

void Proto::handleItemGroupUpdate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->itemGroup().update( params );
    fillAnswer( answer, result );
}

void Proto::handleItemGroupsList( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->itemGroup().list( params );
    fillAnswer( answer, result );
}

void Proto::handleItemGroupsGet( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->itemGroup().get( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceItemGroupExpand( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().expand( params );
    fillAnswer( answer, result );
}

void Proto::handleDeviceGroupsFind( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.groupOperations()->deviceGroup().find( params );
    fillAnswer( answer, result );
}

void Proto::handlePolicyCreate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.policyOperations()->create( params );
    fillAnswer( answer, result );
}

void Proto::handlePolicyDelete( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.policyOperations()->remove( params );
    fillAnswer( answer, result );
}

void Proto::handlePolicyUpdate( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.policyOperations()->update( params );
    fillAnswer( answer, result );
}

void Proto::handlePolicyGet( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.policyOperations()->get( params );
    fillAnswer( answer, result );
}

void Proto::handlePolicyList( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( getService() );
    auto result = remoteInterface.policyOperations()->list( params );
    fillAnswer( answer, result );
}

bool Proto::hubLogCommand( const std::string& id, const std::string& method, const JsonObject& answer ) {
    if( method.find( "hub." ) != std::string::npos ) {
        if( json_contains( answer, "api" ) && json_get_str( answer, "api" ) != "1.0" ) {
            Log::info( cc::info( "Received new command: " ) + cc::note( method ) + cc::info( ", id: " ) +
                       cc::note( id ) + cc::info( ", api: " ) + cc::note( json_get_str( answer, "api" ) ) );
            return true;
        }
        else {
            Log::info( cc::info( "Received new command: " ) + cc::note( method ) + cc::info( ", id: " ) +
                       cc::note( id ) );
            return true;
        }
    }
    return false;
}

bool Proto::cloudLogCommand( std::string& id,
                             const std::string& method,
                             const JsonObject& message,
                             JsonObject& answer ) {
    if( method.find( "cloud." ) != std::string::npos ) {
        if( id.empty() ) {
            id = Db::genOid();
            json_add_str( answer, "id", id );
        }
        if( json_get_int( message[ "result" ], "version" ) > 1 ) {
            Log::info( cc::info( "Received new command: " ) + cc::note( method ) + cc::info( ", id: " ) +
                       cc::note( id ) + cc::info( ", version: " ) +
                       cc::note( std::to_string( json_get_int( message[ "result" ], "version" ) ) ) );
            return true;
        }
        else {
            Log::info( cc::info( "Received new command: " ) + cc::note( method ) + cc::info( ", id: " ) +
                       cc::note( id ) );
            return true;
        }
    }
    return false;
}

void Proto::handleContactIdsList( rpc::Message& rpcMessage ) {
    HubProtectCommandIdHandler( m_uiService ).handleCommandIdList( rpcMessage );
}

void Proto::handleSecurityConfigList( rpc::Message& rpcMessage ) {
    HubProtectSecurityConfigHandler( m_uiService ).handleSecurityConfigList( rpcMessage );
}

void Proto::filterBypassDevices( std::vector< JsonObject >& devices ) {
    for( auto it = devices.begin(); it != devices.end(); ) {
        auto securityConfigId = json_get_str( *it, "protectConfig" );
        if( !securityConfigId.empty() ) {
            if( !HubProtectSecurityConfigHandler( m_uiService ).isBypassSet( securityConfigId ) ) {
                it = devices.erase( it );
                continue;
            }
        }
        ++it;
    }
}

void Proto::handleCloudLocalDataUpdate( rpc::Message& rpcMessage ) {
    try {
        const auto messageId = rpcMessage.getId();
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();
        const std::string method = rpcMessage.getMethod();

        LOGGER_INFO( "Start processing of command: " + method + " with traceId: " + messageId );

        m_uiService.sendAsyncRequest(
            GroupsNames::CLOUD_DEVICES,
            rpcMessage.getParams(),
            HUB_OFFLINE_CLOUD_DATA_UPDATE,
            [joAnswer, peer, &messageId, &method]( const JsonObject& response ) mutable {
                LOGGER_INFO_RAW( "End processing of command: " + method + " with traceId: " + messageId );
                setErrorIfHas( joAnswer, response );
                joAnswer[ "result" ] = response;
                json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                if( peer ) {
                    peer->sendMessage( joAnswer );
                }
            },
            std::chrono::seconds{ 30 } );
        joAnswer = nullptr;
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( "Processing of command: " + rpcMessage.getMethod() + " with traceId: " + rpcMessage.getId() +
                      "failed with error: " + e.what() );
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void Proto::handleNotificationsSmtpSettingsSet( rpc::Message& rpcMessage ) {
    HubNotificationsHandler( m_uiService ).handleSMTPSettingsSet( rpcMessage );
}

void Proto::handleNotificationsSmtpSettingsGet( rpc::Message& rpcMessage ) {
    HubNotificationsHandler( m_uiService ).handleSMTPSettingsGet( rpcMessage );
}

void Proto::handleNotificationsSmtpSend( rpc::Message& rpcMessage ) {
    HubNotificationsHandler( m_uiService ).handleSMTPSend( rpcMessage );
}

void Proto::handleHubOfflineCloudDataInfoGet( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_uiService.sendRequest(
            GroupsNames::CLOUD_DEVICES, rpcMessage.getParams(), MessageSubClasses::HUB_OFFLINE_CLOUD_DATA_INFO_GET );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

}  // namespace Uid
}  // namespace HubPlatform
