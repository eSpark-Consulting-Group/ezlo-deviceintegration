#include "extensions_manager/plugin_model/plugins_gateway_validator.h"
#include "extensions_manager/plugin_model/model_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include <string>

using namespace std;

using namespace ExtensionsManager::Resources::ConfigFields;

namespace ExtensionsManager {

PluginsGatewayValidatorException::PluginsGatewayValidatorException( const std::string& message, JsonPath& path )
: m_path( path )
, runtime_error( message ) {}

PluginsGatewayValidatorException::PluginsGatewayValidatorException( const PluginsGatewayValidatorException& ex )
: m_path( ex.m_path )
, runtime_error( ex.what() ) {}

JsonPath PluginsGatewayValidatorException::getPath() const {
    return m_path;
}

std::string PluginsGatewayValidator::validateStringValue( const Json& obj, const std::string& key, JsonPath& path ) {
    auto gv = path.makeGuard( key );
    std::string res;
    auto foundIt = obj.find( key );
    if( foundIt != obj.cend() ) {
        Json text = *foundIt;
        if( !text.is_string() ) {
            throw PluginsGatewayValidatorException( "field of type 'string' is expected", path );
        }

        res = text.get< std::string >();
        if( text.empty() ) {
            throw PluginsGatewayValidatorException( "field with empty string", path );
        }
    }
    else {
        throw PluginsGatewayValidatorException( "field is expected", path );
    }

    return res;
}

void PluginsGatewayValidator::validateParams() {
    JsonPath path;
    try {
        validateStringValue( m_configGatewayJson, Meta::Name, path );
        validateStringValue( m_configGatewayJson, Meta::Label, path );
        const auto SetItemValueResponsePolicy = "setItemValueResponsePolicy";
        if( m_configGatewayJson.find( SetItemValueResponsePolicy ) != m_configGatewayJson.cend() ) {
            std::string value = validateStringValue( m_configGatewayJson, SetItemValueResponsePolicy, path );
            auto gv = path.makeGuard( m_configGatewayJson );
            if( !( value == "auto" || value == "custom" ) ) {
                throw PluginsGatewayValidatorException( value + " item value response policy type is not supported",
                                                        path );
            }
        }
    }
    catch( const PluginsGatewayValidatorException& ex ) {
        throw ex;
    }
}

void PluginsGatewayValidator::validateLuaScript( const std::string& script,
                                                 std::function< bool( const std::string ) >& isExistScript,
                                                 bool required,
                                                 JsonPath& path ) {
    bool checkScriptExist = m_configGatewayJson.find( script ) != m_configGatewayJson.cend();
    if( checkScriptExist ) {
        std::string scriptPath = validateStringValue( m_configGatewayJson, script, path ) + ".lua";

        std::string HubPrefix = "HUB:";
        auto prefixLength = HubPrefix.size();
        if( scriptPath.rfind( HubPrefix, 0 ) == 0 ) {
            unsigned last = scriptPath.find( "/" );
            auto prefixId = scriptPath.substr( prefixLength, last - prefixLength );
            if( prefixId != m_id ) {
                throw PluginsGatewayValidatorException( "script: " + scriptPath + " plugin Id doesn't match", path );
            }
        }
        else {
            throw PluginsGatewayValidatorException( "script: " + scriptPath + " has wrong format", path );
        }

        if( !isExistScript( scriptPath ) ) {
            path.pushBack( script );
            throw PluginsGatewayValidatorException( "script: " + scriptPath + " Does not exist", path );
        }
    }
    else if( required ) {
        path.pushBack( script );
        throw PluginsGatewayValidatorException( "field is expected", path );
    }
}

void PluginsGatewayValidator::validateCommands( std::function< bool( const std::string ) >& isExistScript ) {
    JsonPath path;
    std::string configurationType;
    auto gv = path.makeGuard( Root::Gateway );
    auto& gateway = m_configGatewayJson;
    using namespace HubPlatform::CoreD::Model::Resources;

    try {
        validateLuaScript( GatewayFields::ForceRemoveDeviceCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::CheckDeviceCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::SetItemValueCommandFieldName, isExistScript, true, path );
        validateLuaScript( GatewayFields::AddItemDictionaryValueCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::SetItemDictionaryValueCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::RemoveItemDictionaryValueCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::ClearItemDictionaryCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::ResetSettingCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::SetSettingValueCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::SetSettingDictionaryValueCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::UpdateDeviceFirmwareCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::CreateAbstractSubscriptionCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::UpdateAbstractSubscriptionCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::DeleteAbstractSubscriptionCommandFieldName, isExistScript, false, path );
        validateLuaScript( GatewayFields::DispatchAbstractSubscriptionCommandFieldName, isExistScript, false, path );
    }
    catch( const PluginsGatewayValidatorException& ex ) {
        LOGGER_ERROR( "ValidateCommands exception: " + ex.what() );
        throw ex;
    }
}

void PluginsGatewayValidator::process( const JsonObject& config, std::function< bool( std::string ) >& isExistScript ) {
    try {
        JsonPath path;
        auto gv = path.makeGuard( Root::Gateway );
        std::string type;
        if( !json_get( config, Root::Type, type ) ) {
            throw PluginsGatewayValidatorException( "field is expected", path );
        }

        if( type == Root::Gateway ) {
            if( !json_get( config, Root::Id, m_id ) ) {
                throw PluginsGatewayValidatorException( "field is expected", path );
            }
            if( !json_get_obj( config, Root::Gateway, m_configGatewayJson ) ) {
                throw PluginsGatewayValidatorException( "field is expected", path );
            }

            validateParams();
            validateCommands( isExistScript );
        }
    }
    catch( const PluginsGatewayValidatorException& ex ) {
        LOGGER_ERROR( "PluginsGatewayValidator exception: " + ex.what() );
        throw;
    }
}

}  // namespace ExtensionsManager
