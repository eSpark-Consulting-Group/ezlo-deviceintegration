#pragma once

#include "uid/extensions/cameras/cameras_request_info.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Cameras {

struct AudioStreamInfo {
    std::string streamId;
    RequestInfo requestInfo;
    JsonObject userData;

    // json = {
    //      "streamId": "<id_string>"
    //      "request": { ... }
    //      "user_data": ...
    // }
    static AudioStreamInfo fromJson( const JsonObject& json ) {
        AudioStreamInfo info;
        // TODO: Check validity
        info.streamId = json_get_str( json, "streamId" );
        info.requestInfo = RequestInfo::fromJson( json[ "request" ] );
        if( json.count( "user_data" ) ) {
            info.userData = json[ "user_data" ];
        }
        return info;
    }

    Json toJson() const {
        Json info;
        info[ "streamId" ] = streamId;
        info[ "request" ] = requestInfo.toJson();
        if( !userData.is_null() ) {
            info[ "user_data" ] = userData;
        }
        return info;
    }
};

}  // namespace Cameras
}  // namespace HubPlatform
