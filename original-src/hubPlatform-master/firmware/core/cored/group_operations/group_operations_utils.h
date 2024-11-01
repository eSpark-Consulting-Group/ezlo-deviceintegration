#pragma once
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
#include "config_reader/config_reader.h"

class IDbClient;
namespace HubPlatform {
namespace CoreD {
namespace GroupOperationsUtils {

bool areObjectArraysEqual( const JsonArray& leftArray, const JsonArray& rightArray );
bool areStringArraysEqual( const JsonArray& leftArray, const JsonArray& rightArray );
void ensurePredefinedItemGroupsInDb( IDbClient& dbClient, const ConfigReader& configReader );
void ensurePredefinedDeviceGroupsInDb( IDbClient& dbClient, const ConfigReader& configReader );

}  // namespace GroupOperationsUtils
}  // namespace CoreD
}  // namespace HubPlatform
