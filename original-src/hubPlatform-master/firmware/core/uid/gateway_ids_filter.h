#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <unordered_set>

namespace HubPlatform {
namespace Uid {

class GatewayIdsFilter final {
public:
    explicit GatewayIdsFilter( const Json& rpcParams );
    bool checkGatewayId( const std::string& gatewayId ) const;
    bool empty() const;

private:
    std::unordered_set< std::string > m_gatewayIds;
};

}  // namespace Uid
}  // namespace HubPlatform
