#pragma once

#include "um_update_operation.h"

#include "json_utils/json_fwd.h"

#include <vector>

namespace HubPlatform {
namespace UpdateManager {

struct UpdatePlan {
    static UpdatePlan fromJson( const JsonObject& json );
    JsonObject toJson() const;

    bool isValid() const;

    static bool validateCustomPluginActions( const JsonObject& json,
                                             std::string& errorMessage,
                                             std::string& errorField );

public:
    std::vector< UpdateOperation > steps;
};

}  // namespace UpdateManager
}  // namespace HubPlatform