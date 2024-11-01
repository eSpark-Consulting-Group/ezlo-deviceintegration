#include "extensions_manager/plugin_model/plugins_metedata_validator.h"
#include "extensions_manager/plugin_model/model_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/dbid.h"

#include "log_utils/ezlo_log.h"
#include "base_utils/utils.h"

#include <string>

using namespace std;

using namespace ExtensionsManager::Resources::ConfigFields;

namespace ExtensionsManager {

PluginsMetadataValidatorException::PluginsMetadataValidatorException( const std::string& message, JsonPath& path )
: m_path( path )
, runtime_error( message ) {}

PluginsMetadataValidatorException::PluginsMetadataValidatorException( const std::string& message )
: runtime_error( message ) {}

PluginsMetadataValidatorException::PluginsMetadataValidatorException( const PluginsMetadataValidatorException& ex )
: m_path( ex.m_path )
, runtime_error( ex.what() ) {}

JsonPath PluginsMetadataValidatorException::getPath() const {
    return m_path;
}

PluginsMetadataValidator::PluginsMetadataValidator() {
    m_supportTypes = { Types::String, Types::Boolean, Types::Int, Types::Float };
    m_supportMetadataNodeTypes = { Values::Node };
    m_supportConfigurationTypes = { Values::None, Values::Static };
}

bool PluginsMetadataValidator::isSupportedType( const std::string& type ) {
    return m_supportTypes.count( type ) == 1;
}

bool PluginsMetadataValidator::isSupportedConfigurationType( const std::string& type ) {
    return m_supportConfigurationTypes.count( type ) == 1;
}

bool PluginsMetadataValidator::isSupportedMetadataNodeType( const std::string& type ) {
    return m_supportMetadataNodeTypes.count( type ) == 1;
}

JsonObject PluginsMetadataValidator::loadConfigJsonString( std::istream& configStream ) {
    JsonPath path;
    try {
        m_configJson = Json::parse( configStream );
    }
    catch( const std::exception& ex ) {
        throw PluginsMetadataValidatorException( ( std::string ) "Plugin config is bad or not exist: " + ex.what(),
                                                 path );
    }

    auto gv = path.makeGuard( Root::Meta );
    if( !json_get_obj( m_configJson, Root::Meta, m_configMetaJson ) ) {
        throw PluginsMetadataValidatorException( "field is expected", path );
    }

    return m_configMetaJson;
}

JsonObject PluginsMetadataValidator::loadInterfacesJsonString( std::istream& interfacesStream ) {
    JsonPath path;
    try {
        m_interfaceJson = Json::parse( interfacesStream );
    }
    catch( const std::exception& ex ) {
        throw PluginsMetadataValidatorException( ( std::string ) "Plugin interface is bad or not exist: ", path );
    }

    if( !json_get_obj( m_interfaceJson, Root::Configuration, m_interfaceConfigurationJson ) ) {
        throw PluginsMetadataValidatorException( "field is expected", path );
    }

    return m_interfaceJson;
}

std::string PluginsMetadataValidator::validateTextObject( const Json& src, const std::string& key, JsonPath& path ) {
    auto gv = path.makeGuard( key );
    std::string res;
    JsonObject obj;
    JsonObject text;
    if( json_get_obj( src, key, obj ) ) {
        auto gv = path.makeGuard( Meta::Text );

        if( obj.find( Meta::Text ) == obj.end() ) {
            throw PluginsMetadataValidatorException( "field is expected", path );
        }

        text = obj[ Meta::Text ];
        if( !text.is_string() ) {
            throw PluginsMetadataValidatorException( "field of type 'string' is expected", path );
        }
    }
    else {
        throw PluginsMetadataValidatorException( "field is expected", path );
    }

    res = text.get< std::string >();
    if( text.empty() ) {
        throw PluginsMetadataValidatorException( "field with empty string", path );
    }

    return res;
}

std::string PluginsMetadataValidator::validateStringValue( const Json& obj,
                                                           const std::string& key,
                                                           JsonPath& path,
                                                           bool isOptional ) {
    auto gv = path.makeGuard( key );
    std::string res;
    auto foundIt = obj.find( key );
    if( foundIt != obj.cend() ) {
        Json text = *foundIt;
        if( !text.is_string() ) {
            throw PluginsMetadataValidatorException( "field of type 'string' is expected", path );
        }

        res = text.get< std::string >();
        if( text.empty() && !isOptional ) {
            throw PluginsMetadataValidatorException( "field with empty string", path );
        }
    }
    else {
        if( !isOptional ) {
            throw PluginsMetadataValidatorException( "field is expected", path );
        }
    }

    return res;
}

int PluginsMetadataValidator::validateIntValue( const Json& obj, const std::string& key, JsonPath& path ) {
    auto gv = path.makeGuard( key );
    if( obj.find( key ) != obj.cend() ) {
        Json val = obj[ key ];
        if( !val.is_number_integer() ) {
            throw PluginsMetadataValidatorException( " field of type 'int' is expected", path );
        }
        return val.get< JsonObject::number_integer_t >();
    }
    throw PluginsMetadataValidatorException( "field of type 'int' is expected", path );
}

bool PluginsMetadataValidator::validateBoolValue( const Json& obj, const std::string& key, JsonPath& path ) {
    auto gv = path.makeGuard( key );

    if( obj.find( key ) != obj.cend() ) {
        Json val = obj[ key ];
        if( !val.is_boolean() ) {
            throw PluginsMetadataValidatorException( "field of type 'bool' is expected", path );
        }
        return val.get< JsonObject::boolean_t >();
    }
    throw PluginsMetadataValidatorException( "field is expected", path );
}

void PluginsMetadataValidator::validateConfigAndGetItsData( JsonObject& config ) {
    JsonPath path;

    std::string name;
    std::string prefix;
    std::string author;
    std::string version;
    std::string type;
    std::string description;
    std::string pluginId;  // value of config.json/id property

    int size;
    JsonObject res;
    string fullName;

    try {
        auto gv = path.makeGuard( "config.json" );
        //  validate root properties
        pluginId = validateStringValue( m_configJson, Root::Id, path );
        version = validateStringValue( m_configJson, Root::Version, path );

        // validate root/meta properties
        path.pushBack( Root::Meta );
        // metaName
        name = validateTextObject( m_configMetaJson, Meta::Name, path );
        description = validateTextObject( m_configMetaJson, Meta::Description, path );
        author = validateTextObject( m_configMetaJson, Meta::Author, path );
        const bool isOptional = true;
        prefix = validateStringValue( m_configMetaJson, Meta::Prefix, path, isOptional );
        {
            JsonObject placement;
            if( json_get( m_configMetaJson, Meta::Placement, placement ) ) {
                if( placement.is_array() || placement.is_object() ) {
                    auto gv = path.makeGuard( Meta::Placement );
                    if( placement.is_array() ) {
                        JsonArray placementArr;
                        if( json_get_arr( placement, Meta::Placement, placementArr ) ) {
                            for( auto item : placementArr ) {
                                if( !item.is_string() ) {
                                    throw PluginsMetadataValidatorException(
                                        "field of type 'Array' should contains only 'string' items" );
                                }
                            }
                        }
                    }
                }
                else {
                    throw PluginsMetadataValidatorException( "field of type 'Array' or 'Object' is expected", path );
                }
            }

            {
                auto gv = path.makeGuard( Meta::Placement );
                if( json_get_obj( m_configMetaJson, Meta::Placement, placement ) ) {
                    for( auto it = placement.begin(); it != placement.end(); ++it ) {
                        auto gv = path.makeGuard( it.key() );
                        if( !it.value().is_boolean() ) {
                            throw PluginsMetadataValidatorException( "field of type 'bool' is expected", path );
                        }
                    }

                    config[ DbFields::Placement ] = placement;
                }
            }
        }

        if( m_configMetaJson.find( Meta::Size ) != m_configMetaJson.cend() ) {
            size = validateIntValue( m_configMetaJson, Meta::Size, path );
            if( size <= 0 ) {
                throw PluginsMetadataValidatorException( std::to_string( size ) + " incorrect file size.", path );
            }
        }

        const auto& type = validateStringValue( m_configMetaJson, Meta::Type, path );
        {
            auto gv = path.makeGuard( Meta::Type );
            if( !isSupportedMetadataNodeType( type ) ) {
                throw PluginsMetadataValidatorException( "'" + type + "' type is not supported", path );
            }
        }

        const auto& language = validateStringValue( m_configMetaJson, Values::Language, path );
        {
            auto gv = path.makeGuard( Values::Language );
            if( language != "lua" ) {
                throw PluginsMetadataValidatorException( "'" + language + "' language is not supported", path );
            }
        }

        m_pluginScriptsLanguage = language;

        std::string shortPluginName = makeShortPluginName( pluginId, prefix );

        using namespace Resources::ConfigFields;
        // serialize config.json to place to db
        config = { { db::ID_FIELD_NAME, db::DbId::genOID() },
                   { DbFields::Plugin, pluginId },
                   { Root::Version, version },
                   { DbFields::MetaName, name },
                   { DbFields::ShortName, shortPluginName },
                   { DbFields::MetaDescription, description },
                   { DbFields::MetaAuthor, author },
                   { DbFields::MetaPrefix, prefix },
                   { DbFields::MetaSize, size },
                   { DbFields::MetaType, type },
                   { DbFields::MetaLanguage, language } };
    }
    catch( const PluginsMetadataValidatorException& ex ) {
        throw ex;
    }
}

std::string PluginsMetadataValidator::makeShortPluginName( const string& pluginId, const string& prefix ) {
    if( prefix.empty() || pluginId.empty() )
        return pluginId;

    string shortName = pluginId;
    string pattern = prefix + "_";
    auto pos = shortName.find( pattern );
    if( pos == 0 ) {
        shortName.replace( pos, pattern.size(), "" );
    }
    return shortName;
}

void PluginsMetadataValidator::validateInterfaces( std::function< bool( const std::string ) >& isExistScript ) {
    JsonPath path;
    std::string configurationType;
    std::string scriptId;
    auto gv = path.makeGuard( "interface.json" );
    auto& configuration = m_interfaceConfigurationJson;
    try {
        auto gv = path.makeGuard( Root::Configuration );
        if( configuration.find( Configuration::Type ) != configuration.cend() ) {
            configurationType = validateStringValue( configuration, Configuration::Type, path );
            auto gv = path.makeGuard( Configuration::Type );

            if( !isSupportedConfigurationType( configurationType ) ) {
                throw PluginsMetadataValidatorException( configurationType + " configuration type is not supported",
                                                         path );
            }
        }
        else {
            path.pushBack( Configuration::Type );
            throw PluginsMetadataValidatorException( "field is expected", path );
        }

        bool checkScriptExist = configuration.find( Configuration::Script ) != configuration.cend();
        if( checkScriptExist ) {
            scriptId = validateStringValue( configuration, Configuration::Script, path );
            if( !isExistScript( scriptId + "." + m_pluginScriptsLanguage ) ) {
                path.pushBack( Configuration::Script );
                throw PluginsMetadataValidatorException( "script: " + scriptId + " Does not exist", path );
            }
        }

        if( configuration.find( Configuration::Inputs ) != configuration.cend() ) {
            auto gv = path.makeGuard( Configuration::Inputs );
            std::string key = Configuration::Inputs;
            JsonArray inputs;

            if( !configuration[ Configuration::Inputs ].is_array() ) {
                throw PluginsMetadataValidatorException( "field of type 'array' is expected", path );
            }

            json_get_arr( configuration, Configuration::Inputs, inputs );

            for( const auto& input : inputs ) {
                validateStringValue( input, Configuration::Name, path );
                validateStringValue( input, Configuration::Description, path );
                validateStringValue( input, Configuration::Field, path );
                validateBoolValue( input, Configuration::Required, path );

                const auto type = validateStringValue( input, Configuration::Type, path );

                if( !isSupportedType( type ) ) {
                    auto gv = path.makeGuard( Configuration::Type );
                    throw PluginsMetadataValidatorException( type + " type is not supported", path );
                }

                const bool isOptional = true;
                validateStringValue( input, Configuration::Format, path, isOptional );
            }
        }

        auto meta = JSON( db::ID_FIELD_NAME << db::DbId::genOID() << Configuration::Type << configurationType
                                            << Configuration::ScriptId << scriptId );
    }
    catch( const PluginsMetadataValidatorException& ex ) {
        throw ex;
    }
}

JsonObject PluginsMetadataValidator::parse( std::istream& config,
                                            std::istream& interfaces,
                                            std::function< bool( std::string ) >& func ) {
    std::string fullName;
    std::string author;

    JsonObject obj;

    try {
        loadConfigJsonString( config );
        validateConfigAndGetItsData( obj );
        JsonObject interfacesJson = loadInterfacesJsonString( interfaces );

        validateInterfaces( func );
        if( !m_interfaceConfigurationJson.empty() ) {
            obj[ Root::Configuration ] = m_interfaceConfigurationJson;
        }
    }
    catch( const PluginsMetadataValidatorException& ex ) {
        throw;
    }

    return obj;
}

JsonObject PluginsMetadataValidator::getConfig() const {
    return m_configJson;
}

JsonObject PluginsMetadataValidator::getInterface() const {
    return m_interfaceJson;
}

}  // namespace ExtensionsManager
