#pragma once

#include "uid/extensions/cameras/cameras_request_hv_info.h"
#include "uid/extensions/cameras/cameras_request_fc_info.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Cameras {

struct RequestInfo {
    enum class Format { Unknown, Hikvision, Foscam };

    Format format = Format::Unknown;

    std::unique_ptr< RequestFCInfo > fc;
    std::unique_ptr< RequestHVInfo > hv;

    // json = {
    //      "format": "fc",
    //      ...
    // }
    static RequestInfo fromJson( const JsonObject& json ) {
        RequestInfo info;
        // TODO: Check validity
        info.format = formatFromString( json_get_str( json, "format" ) );
        switch( info.format ) {
        case Format::Hikvision:
            info.hv = RequestHVInfo::fromJson( json );
        case Format::Foscam:
            info.fc = RequestFCInfo::fromJson( json );
        }
        return info;
    }

    Json toJson() const {
        Json info;
        switch( format ) {
        case Format::Hikvision:
            info = hv->toJson();
            break;
        case Format::Foscam:
            info = fc->toJson();
            break;
        }
        info[ "format" ] = formatToString( format );
        return info;
    }

    static Format formatFromString( const std::string& format ) {
        if( format == "hv" || format.empty() )
            return Format::Hikvision;
        if( format == "fc" )
            return Format::Foscam;

        return Format::Unknown;
    }

    static std::string formatToString( const Format& format ) {
        switch( format ) {
        case Format::Hikvision:
            return "hv";
        case Format::Foscam:
            return "fc";
        }
        return "unknown";
    }
};

}  // namespace Cameras
}  // namespace HubPlatform
