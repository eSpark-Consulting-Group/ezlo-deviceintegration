#pragma once

#include "cored/db_modifier/db_modifier.h"

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class RoomDbModifier final : public DbModifier {
public:
    RoomDbModifier( IDbClient& db, Broadcaster broadcaster );
};

}  // namespace CoreD
}  // namespace HubPlatform
