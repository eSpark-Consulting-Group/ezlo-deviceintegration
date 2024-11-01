#include "zigbee/model/enums/node_type.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct NodeTypeData {
    NodeType typeId;
    const char* name;
};

static const NodeTypeData nodeTypeData[] = {
    { NodeType::Unknown, Resources::Fields::Command::NodeType::Unknown },
    { NodeType::Coordinator, Resources::Fields::Command::NodeType::Coordinator },
    { NodeType::Router, Resources::Fields::Command::NodeType::Router },
    { NodeType::EndDevice, Resources::Fields::Command::NodeType::EndDevice },
    { NodeType::SleepyEndDevice, Resources::Fields::Command::NodeType::SleepyEndDevice }
};

const char* toString( NodeType type ) {
    for( const auto& data : nodeTypeData ) {
        if( data.typeId == type ) {
            return data.name;
        }
    }

    assert( !"Unsupported node type" );

    return Resources::Fields::Command::NodeType::Unknown;
}

NodeType nodeTypeFromString( const std::string& type ) {
    for( const auto& data : nodeTypeData ) {
        if( type == data.name ) {
            return data.typeId;
        }
    }

    assert( !"Unsupported node type name" );

    return NodeType::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
