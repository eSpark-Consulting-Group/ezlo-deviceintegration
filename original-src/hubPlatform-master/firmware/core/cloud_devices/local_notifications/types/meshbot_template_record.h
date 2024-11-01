#pragma once

#include "json_utils/json_fwd.h"
#include <vector>

namespace HubPlatform {
namespace CloudDevices {

struct MeshbotTemplateRecord {
    std::string uuid;
    std::string version;
    std::string title;
    std::string description;
    std::string id;
    std::string oemId;
    std::string partnerUUID;
    std::string localMeshbot;
    JsonArray rules;

    Json toJson() const;
    static MeshbotTemplateRecord fromJson( const Json& j );
};

}  // namespace CloudDevices
}  // namespace HubPlatform
