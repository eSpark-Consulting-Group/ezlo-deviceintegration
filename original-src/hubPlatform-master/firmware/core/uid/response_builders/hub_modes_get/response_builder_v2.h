#pragma once

#include "uid/response_builders/hub_modes_get/response_builder_v1.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V2 {

class ResponseBuilder : public V1::ResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message, const std::string& api );
    JsonObject buildResponse( IDbClient& dbClient ) const;
    JsonArray createAlarmedObject( IDbClient& dbClient ) const;
};

}  // namespace V2
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform