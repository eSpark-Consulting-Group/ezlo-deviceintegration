#pragma once

#include "zigbee/model/zigbee_cluster.h"

#include "json_utils/json_fwd.h"

#include <map>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeEndpoint {
public:
    using ClusterBaseUPtr = std::unique_ptr< ZigbeeClusterBase >;
    using Clusters = std::map< uint16_t, ClusterBaseUPtr >;

public:
    ZigbeeEndpoint( const ZigbeeEndpoint& ) = delete;
    ZigbeeEndpoint( ZigbeeEndpoint&& fromEndpoint ) = delete;
    explicit ZigbeeEndpoint( uint8_t id, uint16_t profileId, uint16_t deviceId );
    explicit ZigbeeEndpoint( const Json& json );
    ~ZigbeeEndpoint() = default;

    void addCluster( ClusterBaseUPtr&& cluster );

    uint8_t getId() const;
    uint16_t getProfileId() const;
    uint16_t getDeviceId() const;

    bool forEachCluster( std::function< bool( ZigbeeClusterBase& ) > callback );
    ZigbeeClusterBase* findCluster( uint16_t clusterId );
    const ZigbeeClusterBase* findCluster( uint16_t clusterId ) const;

    Json toJson() const;

private:
    uint8_t m_id;
    uint16_t m_profileId;
    uint16_t m_deviceId;
    Clusters m_clusters;
};

}  // namespace Zigbee
}  // namespace HubPlatform
