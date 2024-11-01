#pragma once
#include "json_utils/json_fwd.h"
#include <vector>
#include <string>

namespace HubPlatform {
namespace HttpBridge {

class HttpBridgeCommand {
public:
    using Param = std::pair< std::string, std::string >;
    using Params = std::vector< Param >;

public:
    HttpBridgeCommand() = default;

    bool parse( const std::string& url );
    const std::string& method() const;
    const Params& params() const;
    const std::string& version() const;
    void appendParam( std::string key, std::string value );
    std::string findParam( const std::string& key );

    Json toJson() const noexcept;

private:
    bool parseMethod( const std::string& path, unsigned shift );
    bool parseParams( const std::string& params );
    bool parseVersion( std::string version );
    Json prepareParams();

private:
    static std::string stripKeySuffix( const std::string& key, const char* suffix );
    static std::string isBool( const std::string& key );
    static std::string isInt( const std::string& key );
    static std::string isFloat( const std::string& key );
    static bool checkBoolValue( const std::string& key, const std::string& value );
    static int checkIntValue( const std::string& key, const std::string& value );
    static float checkFloatValue( const std::string& key, const std::string& value );

private:
    std::string m_method;
    Params m_params;
    Json m_preparedParams = Json::object();
    std::string m_version{ "1.0" };
};

}  // namespace HttpBridge
}  // namespace HubPlatform