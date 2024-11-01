#pragma once

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Cameras {

struct RequestHVInfo {
    enum class Type { GET, PUT, POST, DELETE };

    std::string url;
    std::string user;
    std::string password;
    Type type = Type::PUT;
    std::vector< std::pair< std::string, std::string > > headers;

    // json = {
    //      "url": "http://192.168.42.55:3580/ISAPI/System/TwoWayAudio/channels/1/audioData",
    //      "user": "admin",
    //      "password": "85000679",
    //      "type": "PUT",
    //      "headers": {
    //          "Content-Type": "application/octet-stream",
    //          "Content-Length": "0"
    //      }
    // }
    static std::unique_ptr< RequestHVInfo > fromJson( const JsonObject& json ) {
        auto info = std::make_unique< RequestHVInfo >();
        // TODO: Check validity
        info->url = json_get_str( json, "url" );
        info->user = json_get_str( json, "user" );
        info->password = json_get_str( json, "password" );
        info->type = typeFromString( json_get_str( json, "type" ) );
        JsonObject headersJson;
        if( json_get_obj( json, "headers", headersJson ) ) {
            for( auto it = headersJson.cbegin(); it != headersJson.cend(); ++it ) {
                if( it.value().is_string() ) {
                    info->headers.emplace_back( it.key(), it.value().get< std::string >() );
                }
            }
        }
        return info;
    }

    Json toJson() const {
        Json info;
        info[ "url" ] = url;
        if( !user.empty() ) {
            info[ "user" ] = user;
        }
        if( !password.empty() ) {
            info[ "password" ] = password;
        }
        info[ "type" ] = getTypeString();
        if( !headers.empty() ) {
            for( auto& header : headers ) {
                info[ "headers" ][ header.first ] = header.second;
            }
        }
        return info;
    }

    std::string getTypeString() const {
        return typeToString( type );
    }

    static Type typeFromString( const std::string& type ) {
        if( type == "GET" )
            return Type::GET;
        if( type == "PUT" )
            return Type::PUT;
        if( type == "POST" )
            return Type::POST;
        if( type == "DELETE" )
            return Type::DELETE;

        return Type::PUT;
    }

    static std::string typeToString( const Type& type ) {
        switch( type ) {
        case Type::GET:
            return "GET";
        case Type::PUT:
            return "PUT";
        case Type::POST:
            return "POST";
        case Type::DELETE:
            return "DELETE";
        }
        return "PUT";
    }
};

}  // namespace Cameras
}  // namespace HubPlatform
