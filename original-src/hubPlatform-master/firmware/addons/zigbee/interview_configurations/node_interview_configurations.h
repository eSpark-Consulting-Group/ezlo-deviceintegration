#pragma once

#include "json_utils/json_utils.h"
#include "base_utils/optional.h"

#include <map>
#include <set>

namespace HubPlatform {
namespace Zigbee {

class NodeInterviewConfigurations {
public:
    using ClusterIdentifier = std::pair< uint16_t, uint16_t >;
    using OptionalClusterIdentifier = common::optional< ClusterIdentifier >;

    static std::unique_ptr< NodeInterviewConfigurations > create( const Json& configurations,
                                                                  const std::string& manufacturerId,
                                                                  const std::string& modelId );

    common::optional< uint16_t > getMfgCode() const;
    bool shouldDiscoverMfgInfo( uint16_t endpointId, uint16_t clusterId ) const;

    bool hasClusterInterviewConfigurations( uint16_t endpointId, uint16_t clusterId ) const;
    bool fullyIgnoreCluster( uint16_t endpointId, uint16_t clusterId ) const;
    bool discoverClusterAttributes( uint16_t endpointId, uint16_t clusterId ) const;
    bool discoverClusterCommandsReceived( uint16_t endpointId, uint16_t clusterId ) const;
    bool discoverClusterCommandsGenerated( uint16_t endpointId, uint16_t clusterId ) const;
    std::vector< uint16_t > getClusterAttributesToRequest( uint16_t endpointId, uint16_t clusterId ) const;
    bool forEachClusterAttributesToRequest( uint16_t endpointId,
                                            uint16_t clusterId,
                                            std::function< bool( uint16_t ) > callback ) const;

private:
    struct ClusterInfo {
        bool m_discoverAttributes = true;
        std::vector< uint16_t > m_attributesToRequest;

        bool m_discoverCommandsReceived = true;
        bool m_discoverCommandsGenerated = true;

        bool m_ignore = false;
    };

    NodeInterviewConfigurations( const Json& nodeConfigurations );

    static std::string getDeviceTypeId( const std::string& manufacturerId, const std::string& modelId );

    void parseConfigurations();
    void parseMfgCode();
    void parseMfgClusters();
    void parseClusterConfigurations();
    OptionalClusterIdentifier parseClusterIdentifier( const Json& data ) const;

    const ClusterInfo* findClusterInfo( uint16_t endpointId, uint16_t clusterId ) const;

private:
    Json m_configurations;

    common::optional< uint16_t > m_mfgCode;
    common::optional< std::set< ClusterIdentifier > > m_mfgClusters;

    std::map< ClusterIdentifier, ClusterInfo > m_clusterConfigurations;
};

}  // namespace Zigbee
}  // namespace HubPlatform
