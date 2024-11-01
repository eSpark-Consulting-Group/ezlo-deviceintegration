#include "zwave/script_events/association_updated.h"

#include "zwave/resources/zwave_resources.h"

#include "json_utils/json_utils.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

template < typename GroupInfo >
AssociationUpdated< GroupInfo >::AssociationUpdated( uint8_t nodeId,
                                                     uint8_t channelId,
                                                     uint8_t ccId,
                                                     uint8_t groupId,
                                                     const GroupInfo& groupInfo )
: ValueEvent( Resources::ScriptEvent::Name::AssociationUpdated, nodeId, ccId, channelId )
, m_groupId( groupId )
, m_groupInfo( groupInfo ) {}

template < typename GroupInfo >
Json AssociationUpdated< GroupInfo >::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::GroupId ] = m_groupId;
    result[ Param::GroupNodes ] = std::move( getNodesInfo() );
    return std::move( result );
}

template < typename GroupInfo >
Json AssociationUpdated< GroupInfo >::getNodesInfo() const {
    using namespace Resources::ScriptEvent;

    auto commands = Json::array();
    for( const auto& nodeInfo : m_groupInfo ) {
        commands.push_back( std::move( getFullNodeInfo( nodeInfo ) ) );
    }

    return std::move( commands );
}

template < typename GroupInfo >
Json AssociationUpdated< GroupInfo >::getFullNodeInfo( const std::pair< uint8_t, uint8_t >& info ) const {
    using namespace Resources::ScriptEvent;

    JsonObject nodeInfoJson;
    nodeInfoJson[ Param::NodeId ] = info.first;
    nodeInfoJson[ Param::EndpointId ] = info.second;
    return nodeInfoJson;
}

template < typename GroupInfo >
Json AssociationUpdated< GroupInfo >::getFullNodeInfo( const uint8_t& info ) const {
    using namespace Resources::ScriptEvent;

    JsonObject nodeInfoJson;
    nodeInfoJson[ Param::NodeId ] = info;
    return nodeInfoJson;
}

template class AssociationUpdated< std::vector< uint8_t > >;
template class AssociationUpdated< std::vector< std::pair< uint8_t, uint8_t > > >;

}  // namespace Zwave
}  // namespace HubPlatform
