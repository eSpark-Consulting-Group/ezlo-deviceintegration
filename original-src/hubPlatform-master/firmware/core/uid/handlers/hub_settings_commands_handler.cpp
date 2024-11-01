#include "uid/handlers/hub_settings_commands_handler.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "uid/ui_service.h"

namespace HubPlatform {
namespace Uid {

HubSettingsCommandsHandler::HubSettingsCommandsHandler( Uid::Service& hostService )
: m_core( CoreDRemoteInterface::makeInstance( hostService ) ) {}

//  "params": {
//      "names": [ "scale.temperature", "time.format" ]
//  }
void HubSettingsCommandsHandler::handleHubSettingsList( rpc::Message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::set< std::string > filter;
    JsonArray names;
    if( json_get_arr( joParams, "names", names ) ) {
        for( auto& name : names ) {
            if( !name.is_string() ) {
                setError( joAnswer, Errors::WRONG_FIELD );
                return;
            }
            filter.insert( name.get< std::string >() );
        }
    }

    auto settingsJson = JSON_EMPTY_ARR;
    auto settings = m_core->getSettings();
    for( auto& setting : settings ) {
        if( filter.empty() || ( filter.count( setting.getName() ) != 0 ) ) {
            settingsJson.push_back( setting.asJson() );
        }
    }
    joAnswer[ "result" ][ "settings" ] = settingsJson;
}

//  "params": {
//      "name": "scale.temperature",
//      "value": "celsius"
//  }
void HubSettingsCommandsHandler::handleSettingValueSet( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string name;
    Json value;

    if( !getNonEmptyField( params, "name", joAnswer, name, false ) ) {
        return;
    }

    if( !json_get( params, "value", value ) ) {
        JsonObject error_mes;
        Errors::getBadRequest( "value", error_mes );
        setErrorObject( joAnswer, error_mes );
        return;
    }

    auto setting = m_core->findSetting( name );
    if( !setting ) {
        setError( joAnswer, Errors::OBJECT_NOT_FOUND, "Setting not found" );
        return;
    }

    if( !setting->setValue( value ) ) {
        setError( joAnswer, Errors::INVALID_VALUE_RANGE, "Invalid setting value" );
        return;
    }

    m_core->saveSetting( *setting );
}

std::vector< Json > HubSettingsCommandsHandler::getSettingsList() const {
    auto settings = m_core->getSettings();
    std::vector< Json > result;
    result.reserve( settings.size() );
    for( auto& setting : settings ) {
        result.emplace_back( setting.asJson() );
    }
    return result;
}

}  // namespace Uid
}  // namespace HubPlatform
