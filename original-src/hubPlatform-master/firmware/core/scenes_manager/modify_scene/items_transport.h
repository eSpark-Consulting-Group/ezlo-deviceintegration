#pragma once
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace ScenesManager {

class ItemsTransport {
public:
    virtual ~ItemsTransport() = default;
    virtual std::vector< Json > getItemsBy( const JsonObject& condition ) = 0;
};

}  // namespace ScenesManager
}  // namespace HubPlatform