#pragma once

#include "uid/response_builders/hub_modes_get/response_builder_v2.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V3 {

class ResponseBuilder : public V2::ResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api );
    JsonObject buildResponse( IDbClient& dbClient ) const;
    static JsonObject getSystemTestInfo( IDbClient& dbClient );

private:
    static JsonObject createSwingerShutdownObject( IDbClient& dbClient );
};

}  // namespace V3
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform