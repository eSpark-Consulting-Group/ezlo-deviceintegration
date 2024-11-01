#include "zigbee/model/zigbee_node.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/zigbee_log.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"

#include <cassert>
#include <chrono>

namespace HubPlatform {
namespace Zigbee {

ZigbeeNode::ZigbeeNode( const Json& json )
: m_id( json_get_int( json, Resources::Fields::Node::Id ) ) {
    m_security = json[ Resources::Fields::Node::Security ];
    m_isReachable = json[ Resources::Fields::Node::IsReachable ];
    m_isBatteryPowered = json[ Resources::Fields::Node::IsBatteryPowered ];
    m_isSleepyDevice = json_get_bool( json, Resources::Fields::Node::IsSleepy );
    m_modelId = json_get_str( json, Resources::Fields::Node::ModelId );
    m_manufacturerId = json_get_str( json, Resources::Fields::Node::ManufacturerId );
    m_lastUpdateTime = std::chrono::system_clock::time_point(
        std::chrono::seconds( json_get_int( json, Resources::Fields::Node::LastUpdateTime ) ) );
    m_reachableStateRecheckTimeout =
        std::chrono::seconds( json_get_int( json, Resources::Fields::Node::ReachableStateRecheckTimeout ) );

    auto capabilitiesIt = json.find( Resources::Fields::Node::Capabilities );
    if( capabilitiesIt != json.end() && capabilitiesIt->is_number_integer() ) {
        m_capabilities = json_get_uint( *capabilitiesIt );
    }

    for( const auto& joEndpoint : json[ Resources::Fields::Node::Endpoints ] ) {
        auto ep = std::make_unique< ZigbeeEndpoint >( joEndpoint );
        m_endpoints.emplace( std::pair< uint16_t, EndpointUPtr >( ep->getId(), std::move( ep ) ) );
    }
}

ZigbeeNode::ZigbeeNode( const uint16_t nodeId,
                        const bool security,
                        const bool isReachable,
                        const bool isBatteryPowered,
                        const std::string& modelId,
                        const std::string& manufacturerId )
: m_id( nodeId )
, m_security( security )
, m_isReachable( isReachable )
, m_isBatteryPowered( isBatteryPowered )
, m_modelId( modelId )
, m_manufacturerId( manufacturerId )
, m_lastUpdateTime( std::chrono::system_clock::now() )
, m_reachableStateRecheckTimeout( std::chrono::hours( 24 ) ) {}

ZigbeeNode::~ZigbeeNode() {}

void ZigbeeNode::addEndpoint( EndpointUPtr&& fromEndpoint ) {
    auto id = fromEndpoint->getId();
    m_endpoints.emplace( id, std::move( fromEndpoint ) );
}

void ZigbeeNode::setCapabilities( const uint8_t capabilities ) {
    m_capabilities = capabilities;
    m_isBatteryPowered = !( *m_capabilities & 0x04 );
    m_isSleepyDevice = !( *m_capabilities & 0x08 );
}

void ZigbeeNode::setDiscovered( const bool discovered ) {
    m_discovered = discovered;
}

void ZigbeeNode::setManufacturerId( const std::string& newValue ) {
    m_manufacturerId = newValue;
}

void ZigbeeNode::setModelId( const std::string& newValue ) {
    m_modelId = newValue;
}

void ZigbeeNode::setIsReachable( bool value ) {
    m_isReachable = value;
}

void ZigbeeNode::deviceUpdated() {
    m_lastUpdateTime = std::chrono::system_clock::now();
}

void ZigbeeNode::setReachableStateRecheckTimeout( std::chrono::system_clock::duration timeout ) {
    m_reachableStateRecheckTimeout = timeout;
}

uint16_t ZigbeeNode::getId() const {
    return m_id;
}

common::optional< uint8_t > ZigbeeNode::getCapabilities() const {
    return m_capabilities;
}

bool ZigbeeNode::getDiscovered() const {
    return m_discovered;
}

const std::string& ZigbeeNode::getManufacturerId() const {
    return m_manufacturerId;
}

const std::string& ZigbeeNode::getModelId() const {
    return m_modelId;
}

bool ZigbeeNode::isReachable() const {
    return m_isReachable;
}

bool ZigbeeNode::isSleepy() const {
    return m_isSleepyDevice;
}

std::chrono::system_clock::time_point ZigbeeNode::getLastUpdateTime() const {
    return m_lastUpdateTime;
}

std::chrono::system_clock::duration ZigbeeNode::getReachableStateRecheckTimeout() const {
    return m_reachableStateRecheckTimeout;
}

bool ZigbeeNode::forEachEndpoint( std::function< bool( ZigbeeEndpoint& ) > callback ) {
    for( auto& data : m_endpoints ) {
        if( !callback( *data.second ) )
            return false;
    }
    return true;
}

ZigbeeEndpoint* ZigbeeNode::findEndpoint( int16_t endpointId ) {
    auto it = m_endpoints.find( endpointId );
    return it != m_endpoints.end() ? it->second.get() : nullptr;
}

const ZigbeeEndpoint* ZigbeeNode::findEndpoint( int16_t endpointId ) const {
    auto it = m_endpoints.find( endpointId );
    return it != m_endpoints.end() ? it->second.get() : nullptr;
}

Json ZigbeeNode::toJson() const {
    JsonObject result;

    result[ Resources::Fields::Node::Id ] = m_id;
    result[ Resources::Fields::Node::Security ] = m_security;
    result[ Resources::Fields::Node::IsReachable ] = m_isReachable;
    result[ Resources::Fields::Node::IsBatteryPowered ] = m_isBatteryPowered;
    result[ Resources::Fields::Node::IsSleepy ] = m_isSleepyDevice;
    result[ Resources::Fields::Node::ModelId ] = m_modelId;
    result[ Resources::Fields::Node::ManufacturerId ] = m_manufacturerId;
    result[ Resources::Fields::Node::LastUpdateTime ] =
        std::chrono::duration_cast< std::chrono::seconds >( m_lastUpdateTime.time_since_epoch() ).count();
    result[ Resources::Fields::Node::ReachableStateRecheckTimeout ] =
        std::chrono::duration_cast< std::chrono::seconds >( m_reachableStateRecheckTimeout ).count();

    if( m_capabilities ) {
        result[ Resources::Fields::Node::Capabilities ] = *m_capabilities;
    }

    JsonArray jaEndpoints;
    for( const auto& ep : m_endpoints ) {
        jaEndpoints.push_back( ep.second->toJson() );
    }

    result[ Resources::Fields::Node::Endpoints ] = std::move( jaEndpoints );

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
