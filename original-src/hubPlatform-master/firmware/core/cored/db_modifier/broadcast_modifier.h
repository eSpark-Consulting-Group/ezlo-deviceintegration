#pragma once
#include "json_utils/json_fwd.h"
#include <string>
#include <memory>

namespace HubPlatform {
namespace CoreD {

class BroadcastModifier {
public:
    virtual ~BroadcastModifier() = default;
    virtual bool shouldUserBeNotified( const std::string& itemName,
                                       const std::string& deviceCategory,
                                       const std::string& deviceSubcategory,
                                       const Json& broadcastData ) = 0;
    virtual bool prepareUpdateBroadcast( const std::string& itemName,
                                         const std::string& deviceCategory,
                                         const std::string& deviceSubcategory,
                                         Json& broadcastData ) = 0;
};
using BroadcastModifierUPtr = std::unique_ptr< BroadcastModifier >;

}  // namespace CoreD
}  // namespace HubPlatform