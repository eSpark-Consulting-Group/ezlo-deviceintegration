#pragma once

#include "json_utils/json_utils.h"
#include "json_utils/json_path.h"

#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>

namespace ExtensionsManager {
class PluginsMetadataValidatorException : public std::runtime_error {
public:
    PluginsMetadataValidatorException( const std::string& message, JsonPath& path );
    explicit PluginsMetadataValidatorException( const std::string& message );
    PluginsMetadataValidatorException( const PluginsMetadataValidatorException& ex );

    JsonPath getPath() const;

private:
    using std::runtime_error::runtime_error;
    JsonPath m_path;
};

class PluginsMetadataValidator {
public:
    PluginsMetadataValidator();
    ~PluginsMetadataValidator() = default;

    void validateConfigAndGetItsData( JsonObject& config );
    void validateInterfaces( std::function< bool( const std::string ) >& func );
    JsonObject loadConfigJsonString( std::istream& str );
    JsonObject loadInterfacesJsonString( std::istream& str );

    JsonObject parse( std::istream& config, std::istream& interfaces, std::function< bool( std::string ) >& func );

    JsonObject getConfig() const;
    JsonObject getInterface() const;

private:
    std::string validateTextObject( const Json& obj, const std::string& key, JsonPath& path );
    static std::string validateStringValue( const Json& obj,
                                            const std::string& key,
                                            JsonPath& path,
                                            bool isOptional = false );
    int validateIntValue( const Json& obj, const std::string& key, JsonPath& path );
    bool validateBoolValue( const Json& obj, const std::string& key, JsonPath& path );
    bool isSupportedType( const std::string& type );
    bool isSupportedConfigurationType( const std::string& type );
    bool isSupportedMetadataNodeType( const std::string& type );
    static std::string makeShortPluginName( const std::string& pluginId, const std::string& prefix );

private:
    Json m_interfaceJson;
    Json m_interfaceConfigurationJson;

    Json m_configJson;
    Json m_configMetaJson;

    Json m_configMeta;
    std::string m_pluginScriptsLanguage;

    std::set< std::string > m_supportTypes;
    std::set< std::string > m_supportConfigurationTypes;
    std::set< std::string > m_supportMetadataNodeTypes;
};

}  // namespace ExtensionsManager
