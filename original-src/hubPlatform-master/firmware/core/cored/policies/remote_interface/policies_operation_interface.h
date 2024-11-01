#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace CoreD {

class PolicyOperationInterface {
public:
    explicit PolicyOperationInterface( Services::Service& hostService );
    JsonObject create( const JsonObject& params );
    JsonObject remove( const JsonObject& params );
    JsonObject update( const JsonObject& params );
    JsonObject get( const JsonObject& params );
    JsonObject list( const JsonObject& params );

private:
    Services::Service& m_hostService;
};

}  // namespace CoreD
}  // namespace HubPlatform
