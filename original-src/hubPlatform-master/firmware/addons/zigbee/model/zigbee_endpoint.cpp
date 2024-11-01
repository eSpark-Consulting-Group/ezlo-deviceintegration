#include "zigbee/model/zigbee_endpoint.h"
#include "zigbee/zigbee_log.h"

#include "zigbee/resources/zigbee_resources.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Zigbee {

ZigbeeEndpoint::ZigbeeEndpoint( uint8_t id, uint16_t profileId, uint16_t deviceId )
: m_id( id )
, m_profileId( profileId )
, m_deviceId( deviceId ) {}

ZigbeeEndpoint::ZigbeeEndpoint( const Json& json )
: m_id( json_get_int( json, Resources::Fields::Endpoint::Id ) )
, m_profileId( json_get_int( json, Resources::Fields::Endpoint::ProfileId ) )
, m_deviceId( json_get_int( json, Resources::Fields::Endpoint::DeviceId ) ) {
    for( const auto& joCluster : json[ Resources::Fields::Endpoint::Clusters ] ) {
        auto cluster = std::make_unique< ZigbeeClusterBase >( joCluster );
        m_clusters.emplace( std::pair< uint16_t, ClusterBaseUPtr >( cluster->getId(), std::move( cluster ) ) );
    }
}

void ZigbeeEndpoint::addCluster( ClusterBaseUPtr&& cluster ) {
    const auto id = cluster->getId();
    m_clusters.emplace( id, std::move( cluster ) );
}

uint8_t ZigbeeEndpoint::getId() const {
    return m_id;
}

uint16_t ZigbeeEndpoint::getDeviceId() const {
    return m_deviceId;
}

uint16_t ZigbeeEndpoint::getProfileId() const {
    return m_profileId;
}

bool ZigbeeEndpoint::forEachCluster( std::function< bool( ZigbeeClusterBase& ) > callback ) {
    for( auto& data : m_clusters ) {
        if( !callback( *data.second ) )
            return false;
    }
    return true;
}

ZigbeeClusterBase* ZigbeeEndpoint::findCluster( uint16_t clusterId ) {
    auto it = m_clusters.find( clusterId );
    return it != m_clusters.end() ? it->second.get() : nullptr;
}

const ZigbeeClusterBase* ZigbeeEndpoint::findCluster( uint16_t clusterId ) const {
    auto it = m_clusters.find( clusterId );
    return it != m_clusters.end() ? it->second.get() : nullptr;
}

Json ZigbeeEndpoint::toJson() const {
    JsonObject result;

    result[ Resources::Fields::Endpoint::Id ] = m_id;
    result[ Resources::Fields::Endpoint::ProfileId ] = m_profileId;
    result[ Resources::Fields::Endpoint::DeviceId ] = m_deviceId;

    JsonArray jaClusters;
    for( const auto& cls : m_clusters ) {
        jaClusters.push_back( cls.second->toJson() );
    }

    result[ Resources::Fields::Endpoint::Clusters ] = std::move( jaClusters );

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
