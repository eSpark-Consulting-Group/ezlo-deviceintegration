#pragma once

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Cameras {

struct RequestFCInfo {
    std::string address;
    int port;
    std::string magic;
    std::string user;
    std::string password;

    // json = {
    //      "address": "192.168.0.212",
    //      "port": 49587,
    //      "magic": "EZLO",
    //      "user": "admin",
    //      "password": "00626EFE2E77"
    // }
    static std::unique_ptr< RequestFCInfo > fromJson( const JsonObject& json ) {
        auto info = std::make_unique< RequestFCInfo >();
        // TODO: Check validity
        info->address = json_get_str( json, "address" );
        info->port = json_get_int( json, "port" );
        info->magic = json_get_str( json, "magic" );
        info->user = json_get_str( json, "user" );
        info->password = json_get_str( json, "password" );
        return info;
    }

    Json toJson() const {
        Json info;
        info[ "address" ] = address;
        info[ "port" ] = port;
        info[ "magic" ] = magic;
        info[ "user" ] = user;
        info[ "password" ] = password;
        return info;
    }
};

}  // namespace Cameras
}  // namespace HubPlatform
