#pragma once
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
class IDbClient;
namespace HubPlatform {
namespace CoreD {
namespace ItemGroupValidator {
JsonObject validateCreateItemGroupRequest( const JsonObject& message, bool checkRequiredParams = true );
};

}  // namespace CoreD
}  // namespace HubPlatform
