#include "uid/gateway_ids_filter.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Uid {

const std::string gatewayIds = "gatewayIds";

GatewayIdsFilter::GatewayIdsFilter( const Json& rpcParams ) {
    JsonArray ids;

    if( json_contains( rpcParams, gatewayIds ) && json_get_arr( rpcParams, gatewayIds, ids ) ) {
        for( auto it = ids.begin(); it != ids.end(); it++ ) {
            if( it.value().is_string() ) {
                m_gatewayIds.emplace( json_get_str( it.value() ) );
            }
        }
    }
}

bool GatewayIdsFilter::checkGatewayId( const std::string& gatewayId ) const {
    return m_gatewayIds.find( gatewayId ) != m_gatewayIds.end();
}

bool GatewayIdsFilter::empty() const {
    return m_gatewayIds.empty();
}

}  // namespace Uid
}  // namespace HubPlatform
