#pragma once

#include <lua.h>
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace LuaD {
namespace CloudStorageModule {
namespace Resources {

namespace Fields {

constexpr char Token[] = "token";
constexpr char TokenExpirationDate[] = "expires_at";
constexpr char XApp[] = "x_app";

namespace Errors {
constexpr char FetchTokenError[] = "Token could not be fetched: %s";
constexpr char FetchEndpointsError[] = "Endpoints could not be fetched: %s";
constexpr char FetchEndpointError[] = "Endpoint could not be fetched: %s";
}  // namespace Errors

}  // namespace Fields
}  // namespace Resources

class CloudStorageModuleError : public std::runtime_error {
public:
    CloudStorageModuleError( const std::string& errDescription )
    : runtime_error( errDescription ) {}
};

class CloudStorage : public Services::Service {
    using TokenData = JsonObject;

public:
    explicit CloudStorage( const std::string& name );
    ~CloudStorage() override;

    TokenData fetchTokenRecord();
    JsonObject fetchEndpoints( const JsonObject& params );
    std::string fetchEndpoint( const std::string& endpointName, const JsonObject& options );

private:
    void tokenJsonDataBuilder( const std::string& token, std::chrono::seconds expireTime, const std::string& xApp );

private:
    Json m_tokenData;
};
}  // namespace CloudStorageModule
}  // namespace LuaD
}  // namespace HubPlatform
