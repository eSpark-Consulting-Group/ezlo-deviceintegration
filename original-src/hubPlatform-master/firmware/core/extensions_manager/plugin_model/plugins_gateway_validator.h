#pragma once

#include "json_utils/json_utils.h"
#include "json_utils/json_path.h"

namespace ExtensionsManager {
class PluginsGatewayValidatorException : public std::runtime_error {
public:
    PluginsGatewayValidatorException( const std::string& message, JsonPath& path );
    PluginsGatewayValidatorException( const PluginsGatewayValidatorException& ex );

    JsonPath getPath() const;

private:
    using std::runtime_error::runtime_error;
    JsonPath m_path;
};

class PluginsGatewayValidator {
public:
    PluginsGatewayValidator() = default;
    ~PluginsGatewayValidator() = default;

    void validateParams();
    void validateCommands( std::function< bool( const std::string ) >& func );
    void process( const JsonObject& config, std::function< bool( std::string ) >& func );

private:
    std::string validateStringValue( const Json& obj, const std::string& key, JsonPath& path );
    void validateLuaScript( const std::string& script,
                            std::function< bool( const std::string ) >& isExistScript,
                            bool required,
                            JsonPath& path );

private:
    std::string m_id;
    Json m_configGatewayJson;
};

}  // namespace ExtensionsManager
