#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

template < typename GroupInfo >
class AssociationUpdated : public ValueEvent {
public:
    AssociationUpdated( uint8_t nodeId, uint8_t channelId, uint8_t ccId, uint8_t groupId, const GroupInfo& groupInfo );

    Json toJson() const override;

private:
    Json getNodesInfo() const;

    Json getFullNodeInfo( const std::pair< uint8_t, uint8_t >& info ) const;
    Json getFullNodeInfo( const uint8_t& info ) const;

    const uint8_t m_groupId;
    GroupInfo m_groupInfo;
};

}  // namespace Zwave
}  // namespace HubPlatform
