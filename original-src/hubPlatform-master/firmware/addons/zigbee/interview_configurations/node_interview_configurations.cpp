#include "zigbee/interview_configurations/node_interview_configurations.h"

namespace HubPlatform {
namespace Zigbee {

std::unique_ptr< NodeInterviewConfigurations > NodeInterviewConfigurations::create( const Json& configurations,
                                                                                    const std::string& manufacturerId,
                                                                                    const std::string& modelId ) {
    auto allDevicesConfigurationsIt = configurations.find( "devices_configurations" );
    if( allDevicesConfigurationsIt == configurations.end() ) {
        Log::trace( "No Zigbee device interview configurations found" );
        return nullptr;
    }

    auto deviceTypeId = getDeviceTypeId( manufacturerId, modelId );

    auto deviceConfigurationIt = allDevicesConfigurationsIt->find( deviceTypeId );
    std::set< std::string > visitedDeviceTypeIds{ deviceTypeId };
    while( deviceConfigurationIt != allDevicesConfigurationsIt->end() ) {
        if( deviceConfigurationIt->is_object() ) {
            return std::unique_ptr< NodeInterviewConfigurations >{ new NodeInterviewConfigurations(
                *deviceConfigurationIt ) };
        }
        else if( deviceConfigurationIt->is_string() &&
                 visitedDeviceTypeIds.emplace( deviceConfigurationIt->get< std::string >() ).second ) {
            deviceConfigurationIt = allDevicesConfigurationsIt->find( deviceConfigurationIt->get< std::string >() );
        }
        else {
            break;
        }
    }

    return nullptr;
}

common::optional< uint16_t > NodeInterviewConfigurations::getMfgCode() const {
    return m_mfgCode;
}

bool NodeInterviewConfigurations::shouldDiscoverMfgInfo( uint16_t endpointId, uint16_t clusterId ) const {
    return m_mfgClusters.has_value() ?
               ( m_mfgClusters->find( std::make_pair( endpointId, clusterId ) ) != m_mfgClusters->end() ) :
               true;
}

bool NodeInterviewConfigurations::hasClusterInterviewConfigurations( uint16_t endpointId, uint16_t clusterId ) const {
    return findClusterInfo( endpointId, clusterId ) != nullptr;
}

bool NodeInterviewConfigurations::fullyIgnoreCluster( uint16_t endpointId, uint16_t clusterId ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    return info != nullptr ? info->m_ignore : false;
}

bool NodeInterviewConfigurations::discoverClusterAttributes( uint16_t endpointId, uint16_t clusterId ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    return info != nullptr ? info->m_discoverAttributes : true;
}

bool NodeInterviewConfigurations::discoverClusterCommandsReceived( uint16_t endpointId, uint16_t clusterId ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    return info != nullptr ? info->m_discoverCommandsReceived : true;
}

bool NodeInterviewConfigurations::discoverClusterCommandsGenerated( uint16_t endpointId, uint16_t clusterId ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    return info != nullptr ? info->m_discoverCommandsGenerated : true;
}

std::vector< uint16_t > NodeInterviewConfigurations::getClusterAttributesToRequest( uint16_t endpointId,
                                                                                    uint16_t clusterId ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    return info != nullptr ? info->m_attributesToRequest : std::vector< uint16_t >();
}

bool NodeInterviewConfigurations::forEachClusterAttributesToRequest(
    uint16_t endpointId,
    uint16_t clusterId,
    std::function< bool( uint16_t ) > callback ) const {
    const auto* info = findClusterInfo( endpointId, clusterId );
    if( info ) {
        for( const auto& attrId : info->m_attributesToRequest ) {
            if( !callback( attrId ) ) {
                return false;
            }
        }
    }

    return true;
}

NodeInterviewConfigurations::NodeInterviewConfigurations( const Json& nodeConfigurations )
: m_configurations( nodeConfigurations ) {
    Log::debug( "Node interview configurations: " + cc::j( nodeConfigurations ) );
    parseConfigurations();
}

std::string NodeInterviewConfigurations::getDeviceTypeId( const std::string& manufacturerId,
                                                          const std::string& modelId ) {
    return manufacturerId + "_" + modelId;
}

void NodeInterviewConfigurations::parseConfigurations() {
    parseMfgCode();
    parseMfgClusters();
    parseClusterConfigurations();
}

void NodeInterviewConfigurations::parseMfgCode() {
    auto mfgCodeIt = m_configurations.find( "mfg_code" );
    if( mfgCodeIt != m_configurations.end() ) {
        m_mfgCode = mfgCodeIt->get< uint16_t >();
    }
}

void NodeInterviewConfigurations::parseMfgClusters() {
    auto mfgClustersIt = m_configurations.find( "mfg_clusters" );
    if( mfgClustersIt != m_configurations.end() ) {
        m_mfgClusters = std::set< ClusterIdentifier >();

        for( const auto& mfgCluster : *mfgClustersIt ) {
            auto clusterIdentifier = parseClusterIdentifier( mfgCluster );
            if( clusterIdentifier ) {
                m_mfgClusters->emplace( *clusterIdentifier );
            }
        }
    }
}

void NodeInterviewConfigurations::parseClusterConfigurations() {
    auto clusterInfosIt = m_configurations.find( "cluster_infos" );
    if( clusterInfosIt != m_configurations.end() ) {
        for( const auto& clusterInfoJson : *clusterInfosIt ) {
            auto clusterIdentifier = parseClusterIdentifier( clusterInfoJson );
            if( !clusterIdentifier ) {
                continue;
            }

            m_clusterConfigurations.emplace( *clusterIdentifier, ClusterInfo() );
            auto& clusterInfo = m_clusterConfigurations[ *clusterIdentifier ];

            auto ignoreIt = clusterInfoJson.find( "ignore" );
            if( ignoreIt != clusterInfoJson.end() ) {
                clusterInfo.m_ignore = ignoreIt->get< bool >();
            }

            auto discoverCommandsReceivedIt = clusterInfoJson.find( "discover_commands_received" );
            if( discoverCommandsReceivedIt != clusterInfoJson.end() ) {
                clusterInfo.m_discoverCommandsReceived = discoverCommandsReceivedIt->get< bool >();
            }

            auto discoverCommandsGeneratedIt = clusterInfoJson.find( "discover_commands_generated" );
            if( discoverCommandsGeneratedIt != clusterInfoJson.end() ) {
                clusterInfo.m_discoverCommandsGenerated = discoverCommandsGeneratedIt->get< bool >();
            }

            auto attributesToRequestIt = clusterInfoJson.find( "attributes_to_request" );
            if( attributesToRequestIt != clusterInfoJson.end() ) {
                clusterInfo.m_attributesToRequest = attributesToRequestIt->get< std::vector< uint16_t > >();
                clusterInfo.m_discoverAttributes = false;
            }
        }
    }
}

NodeInterviewConfigurations::OptionalClusterIdentifier NodeInterviewConfigurations::parseClusterIdentifier(
    const Json& data ) const {
    auto endpointIdIt = data.find( "endpoint_id" );
    auto clusterIdIt = data.find( "cluster_id" );
    if( endpointIdIt != data.end() && clusterIdIt != data.end() ) {
        return std::make_pair( endpointIdIt->get< uint16_t >(), clusterIdIt->get< uint16_t >() );
    }

    return OptionalClusterIdentifier();
}

const NodeInterviewConfigurations::ClusterInfo* NodeInterviewConfigurations::findClusterInfo(
    uint16_t endpointId,
    uint16_t clusterId ) const {
    auto it = m_clusterConfigurations.find( std::make_pair( endpointId, clusterId ) );
    if( it != m_clusterConfigurations.end() ) {
        return &it->second;
    }

    return nullptr;
}

}  // namespace Zigbee
}  // namespace HubPlatform
