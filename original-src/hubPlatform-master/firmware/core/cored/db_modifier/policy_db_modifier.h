#pragma once

#include "cored/db_modifier/db_modifier.h"

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class PolicyDbModifier final : public DbModifier {
public:
    PolicyDbModifier( IDbClient& db, Broadcaster broadcaster );
};

}  // namespace CoreD
}  // namespace HubPlatform
