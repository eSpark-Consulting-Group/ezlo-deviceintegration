#pragma once

#include <jsonrpc/jsonrpc.h>
#include <string>

namespace HubPlatform {
namespace LuaAddon {

struct StatusData {
    StatusData()
    : percentage( 0 )
    , canBeSent( true ) {}
    StatusData( const StatusData& ) = default;
    StatusData( StatusData&& ) = default;
    StatusData& operator=( const StatusData& ) = default;
    StatusData& operator=( StatusData&& ) = default;
    StatusData( std::string status, std::string operationId, int percentage = 0, bool canBeSent = true )
    : status( status )
    , operationId( operationId )
    , percentage( percentage )
    , canBeSent( canBeSent ) {}

    std::string status;
    std::string operationId;
    int percentage;
    bool canBeSent;

    JsonObject toJson() const {
        return JSON( "status" << status << "completed" << percentage << "operationId" << operationId );
    }
};

} /* namespace LuaAddon */
} /* namespace HubPlatform */
