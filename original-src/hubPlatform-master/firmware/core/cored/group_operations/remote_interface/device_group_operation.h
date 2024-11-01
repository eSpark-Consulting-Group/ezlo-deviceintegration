#pragma once
#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreD {

class DeviceGroupOperation {
public:
    explicit DeviceGroupOperation( Services::Service& service );
    JsonObject create( const JsonObject& params );
    JsonObject deleteGroup( const JsonObject& params );
    JsonObject update( const JsonObject& params );
    JsonObject list( const JsonObject& params );
    JsonObject get( const JsonObject& params );
    JsonObject find( const JsonObject& params );
    JsonObject expand( const JsonObject& params );
    JsonObject addDevices( const JsonObject& params );
    JsonObject removeDevices( const JsonObject& params );

private:
    Services::Service& m_hostService;
};

}  // namespace CoreD
}  // namespace HubPlatform
