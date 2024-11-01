#include "cloud_storage_module.h"
#include "cloud_storage/controller_token.h"
#include "binding_interfaces/lua/lua_binding_interface.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace LuaD {
namespace CloudStorageModule {

CloudStorage::CloudStorage( const std::string& name )
: Service( name ) {
    Services::Service::setUp();
}

CloudStorage::TokenData CloudStorage::fetchTokenRecord() {
    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( getDBClient(), controllerToken );

    auto token = controllerToken.getTokenValue();
    if( token.empty() ) {
        throw CloudStorageModuleError( "Token is not present" );
    }
    auto expireTime = controllerToken.getExpirationTime();
    auto xApp = controllerToken.getXAppValue();
    tokenJsonDataBuilder( token, expireTime, xApp );
    return m_tokenData;
}

void CloudStorage::tokenJsonDataBuilder( const std::string& token,
                                         std::chrono::seconds expireTime,
                                         const std::string& xApp ) {
    m_tokenData[ Resources::Fields::Token ] = token;
    m_tokenData[ Resources::Fields::TokenExpirationDate ] = expireTime.count();
    m_tokenData[ Resources::Fields::XApp ] = xApp;
}

JsonObject CloudStorage::fetchEndpoints( const JsonObject& params ) {
    if( params.empty() || !params.is_object() ) {
        throw CloudStorageModuleError( "Parameter should be an object with keys to fetch specific endpoint data" );
    }
    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( getDBClient(), controllerToken );
    const auto& endpoints = controllerToken.getEndpoints();
    Json endPointsTable;

    for( auto it = params.begin(); it != params.end(); ++it ) {
        if( json_get_bool( params, it.key() ) ) {
            auto urls = endpoints.fetchEndpointByName( it.key() );
            endPointsTable[ it.key() ] = urls;
        }
    }

    return endPointsTable;
}

std::string CloudStorage::fetchEndpoint( const std::string& endpointName, const JsonObject& options ) {
    if( endpointName.empty() ) {
        throw CloudStorageModuleError( "Param should be a string name to fetch specific endpoint data" );
    }

    using namespace Common::CloudStorageInterface;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( getDBClient(), controllerToken );
    const auto& endpoints = controllerToken.getEndpoints();
    auto urls = endpoints.fetchEndpointByName( endpointName );
    if( urls.empty() ) {
        throw CloudStorageModuleError( "Url is not present for endpoint: " + endpointName );
    }
    std::string url;
    if( !options.empty() && options.is_object() ) {
        if( json_contains( options, "shuffle" ) && json_get_bool( options, "shuffle" ) ) {
            Endpoints::shuffleSelect( urls, url );
        }
        else if( json_contains( options, "index" ) ) {
            auto index = json_get_int( options, "index", 1 );
            if( index > urls.size() ) {
                throw CloudStorageModuleError( "index out of range: " + std::to_string( index ) );
            }
            url = urls.at( index - 1 );
        }
    }
    else {
        url = urls.front();
    }
    return url;
}

CloudStorage::~CloudStorage() = default;

}  // namespace CloudStorageModule
}  // namespace LuaD
}  // namespace HubPlatform