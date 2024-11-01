#pragma once

#include <string>
#include "json_utils/json_fwd.h"

class IDbClient;

namespace HubPlatform {
namespace CloudDevices {
namespace LocalNotification {
namespace Utils {

void addCloudDataInfo( IDbClient& dbClient,
                       const int version,
                       const int fileVersion,
                       const std::string& url,
                       const std::string& s3Id );

void addMeshbotsDataInfo( IDbClient& dbClient,
                          const int version,
                          const int fileVersion,
                          const std::string& url,
                          const std::string& s3Id );

bool setActiveCloudDataVersion( IDbClient& dbClient, const int version, const int fileVersion );

bool setActiveMeshbotsDataVersion( IDbClient& dbClient, const int version, const int fileVersion );

Json getCloudDataInfo( IDbClient& dbClient, const int version, const int fileVersion );

Json getMeshbotsDataInfo( IDbClient& dbClient, const int version, const int fileVersion );

Json getActiveCloudDataInfo( IDbClient& dbClient );

Json getActiveMeshbotsDataInfo( IDbClient& dbClient );

void removeUnusedCloudDataVersions( IDbClient& dbClient );
void removeUnusedMeshbotsVersions( IDbClient& dbClient );

}  // namespace Utils
}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
