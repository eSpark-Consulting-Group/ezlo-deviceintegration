#include "cored/db_modifier/room_db_modifier.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {

RoomDbModifier::RoomDbModifier( IDbClient& db, HubPlatform::CoreD::DbModifier::Broadcaster broadcaster )
: DbModifier( db,
              broadcaster,
              Model::Resources::DBNames::RoomsDB,
              { { HUB_ROOM_CREATED, {} }, { HUB_ROOM_EDITED, {} }, { HUB_ROOM_DELETED, {} } },
              {} ) {}

}  // namespace CoreD
}  // namespace HubPlatform
