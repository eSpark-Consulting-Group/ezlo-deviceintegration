#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreD {

class ItemGroupUpdater {
public:
    static JsonObject getItemGroupUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData );
    static JsonObject getItemGroupUpdateData( const JsonObject& dbItem, const Json& params );
};

}  // namespace CoreD
}  // namespace HubPlatform
