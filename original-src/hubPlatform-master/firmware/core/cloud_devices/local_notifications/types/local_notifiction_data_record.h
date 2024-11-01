#pragma once

#include "json_utils/json_fwd.h"
#include <vector>

namespace HubPlatform {
namespace CloudDevices {

struct LocalNotificationDataRecord {
    std::string status;
    int currentVersion;
    std::string fileVersion;
    std::string url;
    std::string s3Id;
    uint64_t modified;

    Json toJson() const;
    static LocalNotificationDataRecord fromJson( const Json& j );
};

}  // namespace CloudDevices
}  // namespace HubPlatform
