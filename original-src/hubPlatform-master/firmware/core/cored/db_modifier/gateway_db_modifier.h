#pragma once

#include "cored/db_modifier/db_modifier.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"
#include "cored/what_happened.h"

#include <string>
#include <functional>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class GatewayDbModifier final : public DbModifier {
public:
    GatewayDbModifier( IDbClient& db, Broadcaster broadcaster );
    ~GatewayDbModifier();
};

}  // namespace CoreD
}  // namespace HubPlatform
