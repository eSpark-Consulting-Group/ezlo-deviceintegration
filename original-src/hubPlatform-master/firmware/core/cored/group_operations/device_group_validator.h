#pragma once
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
class IDbClient;
namespace HubPlatform {
namespace CoreD {

class DeviceGroupValidator {
public:
    explicit DeviceGroupValidator( IDbClient& client )
    : m_dbClient( client ) {}
    JsonObject validateCreateGroupRequest( const JsonObject& message, bool checkRequiredParams );

private:
    JsonObject checkDevices( const JsonArray& devices, JsonPath& path );

private:
    IDbClient& m_dbClient;
};

}  // namespace CoreD
}  // namespace HubPlatform
