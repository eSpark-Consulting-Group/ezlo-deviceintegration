#include "zigbee/zigbee_node_manager.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/common/zigbee_exceptions.h"

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "database_client/idbclient.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Zigbee {

NodeManager::NodeManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    if( !m_dbClient.exist( Resources::DB::TableNames::DevicesReachableInited ) ) {
        m_dbClient.create( Resources::DB::TableNames::DevicesReachableInited );
    }
    m_dbClient.clear( Resources::DB::TableNames::DevicesReachableInited );

    const auto jsonNodesData = m_dbClient.findAll( Resources::DB::TableNames::Devices, {}, nullptr );
    for( auto& jsonNodeData : jsonNodesData ) {
        auto node = deserializeNode( jsonNodeData );
        assert( node );
        m_cachedNodes.emplace( node->getId(), std::move( node ) );
    }
}

ZigbeeNode* NodeManager::findNode( uint16_t nodeId ) {
    return NodeManager::findNodeImpl( *this, nodeId );
}

const ZigbeeNode* NodeManager::findNode( uint16_t nodeId ) const {
    return NodeManager::findNodeImpl( *this, nodeId );
}

void NodeManager::add( std::shared_ptr< ZigbeeNode > zigbeeNode ) {
    const Json joZigbeeNode( zigbeeNode->toJson() );
    m_dbClient.insert( Zigbee::Resources::DB::TableNames::Devices, joZigbeeNode );
}

template < typename Manager >
ZigbeeNode* NodeManager::findNodeImpl( Manager& nodeManager, uint16_t nodeId ) {
    {
        yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( nodeManager.m_cacheMutex );
        auto cacheIt = nodeManager.m_cachedNodes.find( nodeId );
        if( cacheIt != nodeManager.m_cachedNodes.end() )
            return cacheIt->second.get();
    }

    const auto jsonNodeData = nodeManager.m_dbClient.findOne(
        Resources::DB::TableNames::Devices, JSON( Resources::DB::Fields::Id << nodeId ), nullptr );
    if( !jsonNodeData.empty() ) {
        auto node = deserializeNode( jsonNodeData );

        assert( node );

        std::unique_lock< yamc::alternate::shared_mutex > lock( nodeManager.m_cacheMutex );
        return nodeManager.m_cachedNodes.emplace( nodeId, std::move( node ) ).first->second.get();
    }

    LOGGER_TRACE( "fail to find a node with id: " + std::to_string( nodeId ) );
    return nullptr;
}

bool NodeManager::remove( uint16_t nodeId ) {
    const auto dev = JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) );
    const auto joNode = m_dbClient.findOne( Resources::DB::TableNames::Devices, dev, nullptr );
    if( joNode.empty() )
        return false;

    m_dbClient.remove( Resources::DB::TableNames::Devices, dev );

    std::unique_lock< yamc::alternate::shared_mutex > lock( m_cacheMutex );
    m_cachedNodes.erase( nodeId );

    return true;
}

bool NodeManager::update( uint16_t nodeId ) {
    auto node = findNode( nodeId );
    if( !node )
        return false;

    m_dbClient.update( Resources::DB::TableNames::Devices,
                       JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) ),
                       serializeNode( *node ),
                       false /*upsert*/,
                       true /*multi*/ );

    return true;
}

void NodeManager::reset() {
    m_dbClient.remove( Resources::DB::TableNames::Devices, JSON_EMPTY_OBJ );

    std::unique_lock< yamc::alternate::shared_mutex > lock( m_cacheMutex );
    m_cachedNodes.clear();
}

void NodeManager::forEachNode( std::function< void( const ZigbeeNode& ) > handler ) const {
    const JsonObject fieldsToReturn = JSON( Resources::DB::Fields::Id << "id" );
    auto ids = m_dbClient.findAll( Resources::DB::TableNames::Devices, JSON_EMPTY_OBJ, &fieldsToReturn );

    for( auto id : ids ) {
        uint16_t nodeId = json_get_uint( id, "id" );
        if( nodeId != 0 ) {
            const auto* node = findNode( nodeId );
            if( node ) {
                handler( *node );
            }
        }
    }
}

void NodeManager::getZigbeeNodeUpdateTimes( JsonObject& zigbeeNodeUpdateTimes ) {
    forEachNode( [&zigbeeNodeUpdateTimes]( const Zigbee::ZigbeeNode& zigbeeNode ) {
        zigbeeNodeUpdateTimes[ std::to_string( zigbeeNode.getId() ) ] = JSON(
            "last_update" << std::chrono::time_point_cast< std::chrono::seconds >( zigbeeNode.getLastUpdateTime() )
                                 .time_since_epoch()
                                 .count()
                          << "reachable" << ( zigbeeNode.isReachable() ) );
    } );
}

bool NodeManager::isBroadcastedAfterReboot( uint16_t nodeId ) const {
    return m_dbClient.has( Zigbee::Resources::DB::TableNames::DevicesReachableInited,
                           JSON( Zigbee::Resources::Fields::Node::Id << nodeId ) );
}

void NodeManager::setBroadcastedAfterReboot( uint16_t nodeId ) {
    m_dbClient.insert( Zigbee::Resources::DB::TableNames::DevicesReachableInited,
                       JSON( Zigbee::Resources::Fields::Node::Id << nodeId ) );
}

Json NodeManager::serializeNode( const ZigbeeNode& node ) {
    return node.toJson();
}

std::unique_ptr< ZigbeeNode > NodeManager::deserializeNode( const Json& jsonNodeData ) {
    return std::make_unique< ZigbeeNode >( jsonNodeData );
}

}  // namespace Zigbee
}  // namespace HubPlatform
