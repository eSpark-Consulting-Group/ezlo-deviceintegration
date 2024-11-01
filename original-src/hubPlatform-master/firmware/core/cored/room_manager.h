#pragma once

#include "json_utils/json_fwd.h"

#include <unordered_map>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class CoreService;

class RoomManager final {
public:
    static constexpr size_t NameMaxLength = 30;
    RoomManager( CoreD::CoreService& hostService );

public:
    JsonObject createRoom( const JsonObject& request );
    JsonObject setRoomName( const JsonObject& request );
    JsonObject setRoomOrder( const JsonObject& request );

    void deleteRoom( const std::string& id );

    std::pair< std::string, std::string > migrateVeraRooms(
        const Json& veraDataModel,
        std::unordered_map< int, std::string >& veraToEzloRoomMappings );

private:
    CoreD::CoreService& m_coreService;
};

}  // namespace CoreD
}  // namespace HubPlatform
