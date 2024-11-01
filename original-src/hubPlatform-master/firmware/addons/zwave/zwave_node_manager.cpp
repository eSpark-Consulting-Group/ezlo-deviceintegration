#include "zwave/zwave_node_manager.h"

#include "znet/znet_reachable_check_timeout_settings.hpp"
#include "zwave/resources/zwave_resources.h"
#include "zwave/common/zwave_exceptions.h"
#include "zwave/serialization/zwave_node_base64_serializer.h"
#include "zwave/common/zwave_exceptions.h"

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "database_client/idbclient.h"
#include "common_utils/cpp14.h"

#include <znet/znet_node.hpp>

namespace HubPlatform {
namespace Zwave {

NodeManager::NodeManager( IDbClient& dbClient,
                          znet::RequestsManager& requestManager,
                          znet::IAssociationsConfigurator& associationsConfigurator,
                          znet::ReachableCheckTimeoutSettings& reachableCheckTimeoutSettings )
: m_dbClient( dbClient )
, m_requestManager( requestManager )
, m_associationsConfigurator( associationsConfigurator )
, m_reachableCheckTimeoutSettings( reachableCheckTimeoutSettings ) {
    const auto jsonNodesData = m_dbClient.findAll( Resources::DB::TableNames::Devices, {}, nullptr );
    for( auto& jsonNodeData : jsonNodesData ) {
        auto node = deserializeNode(
            jsonNodeData, &m_requestManager, &m_associationsConfigurator, &m_reachableCheckTimeoutSettings );
        assert( node );
        m_cachedNodes.emplace( node->getId(), std::move( node ) );
    }
}

NodeManager::~NodeManager() = default;

znet::ZNode* NodeManager::findNode( uint8_t nodeId ) {
    return NodeManager::findNodeImpl( *this, nodeId );
}

const znet::ZNode* NodeManager::findNode( uint8_t nodeId ) const {
    return NodeManager::findNodeImpl( *this, nodeId );
}

void NodeManager::resetNodeCache( uint8_t nodeId ) {
    std::unique_lock< yamc::alternate::shared_mutex > lock( m_cacheMutex );
    auto it = m_cachedNodes.find( nodeId );
    if( it != m_cachedNodes.end() ) {
        auto& node = it->second;
        const auto jsonNodeData = m_dbClient.findOne( Resources::DB::TableNames::Devices,
                                                      JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) ),
                                                      nullptr );

        if( !jsonNodeData.empty() ) {
            node->reset();
            deserializeExistionNode( *node, jsonNodeData );
        }
        else {
            std::unique_ptr< znet::ZNode > tmpNode;

            auto it = m_cachedNodes.find( nodeId );
            if( it != m_cachedNodes.end() ) {
                tmpNode = std::move( it->second );
                m_cachedNodes.erase( it );
            }
            lock.unlock();
        }
    }
}

template < typename Manager >
znet::ZNode* NodeManager::findNodeImpl( Manager& nodeManager, uint8_t nodeId ) {
    {
        yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( nodeManager.m_cacheMutex );
        auto cacheIt = nodeManager.m_cachedNodes.find( nodeId );
        if( cacheIt != nodeManager.m_cachedNodes.end() )
            return cacheIt->second.get();
    }

    const auto jsonNodeData = nodeManager.m_dbClient.findOne(
        Resources::DB::TableNames::Devices, JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) ), nullptr );
    if( !jsonNodeData.empty() ) {
        auto node = deserializeNode( jsonNodeData,
                                     &nodeManager.m_requestManager,
                                     &nodeManager.m_associationsConfigurator,
                                     &nodeManager.m_reachableCheckTimeoutSettings );

        assert( node );

        std::unique_lock< yamc::alternate::shared_mutex > lock( nodeManager.m_cacheMutex );
        return nodeManager.m_cachedNodes.emplace( nodeId, std::move( node ) ).first->second.get();
    }

    LOGGER_TRACE( "fail to find a node with id: " + std::to_string( nodeId ) );
    return nullptr;
}

znet::ZNode& NodeManager::registerNode( uint8_t nodeId ) {
    auto node = std::make_unique< znet::ZNode >(
        &m_requestManager, &m_associationsConfigurator, &m_reachableCheckTimeoutSettings );

    node->init( nodeId );

    m_dbClient.insert( Resources::DB::TableNames::Devices, serializeNode( *node ) );

    std::unique_lock< yamc::alternate::shared_mutex > lock( m_cacheMutex );
    return *m_cachedNodes.emplace( nodeId, std::move( node ) ).first->second;
}

bool NodeManager::remove( uint8_t nodeId ) {
    const auto dev = JSON( Resources::DB::Fields::Id << std::to_string( nodeId ) );
    const auto joNode = m_dbClient.findOne( Resources::DB::TableNames::Devices, dev, nullptr );
    if( joNode.empty() )
        return false;

    m_dbClient.remove( Resources::DB::TableNames::Devices, dev );

    std::unique_lock< yamc::alternate::shared_mutex > lock( m_cacheMutex );
    m_cachedNodes.erase( nodeId );

    return true;
}

bool NodeManager::update( uint8_t nodeId ) {
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

Json NodeManager::serializeNode( const znet::ZNode& node ) {
    JsonObject jsonNodeData;
    jsonNodeData[ Resources::DB::Fields::Id ] = std::to_string( node.getId() );
    jsonNodeData[ Resources::DB::Fields::Base64Data ] = ZwaveNodeBase64Serializer::serialize( node );
    return jsonNodeData;
}

std::unique_ptr< znet::ZNode > NodeManager::deserializeNode(
    const Json& jsonNodeData,
    znet::RequestsManager* requestManager,
    znet::IAssociationsConfigurator* associationsConfigurator,
    znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings ) {
    return ZwaveNodeBase64Serializer::deserialize( jsonNodeData[ Resources::DB::Fields::Base64Data ],
                                                   requestManager,
                                                   associationsConfigurator,
                                                   reachableCheckTimeoutSettings );
}

void NodeManager::deserializeExistionNode( znet::ZNode& node, const Json& jsonNode ) {
    ZwaveNodeBase64Serializer::deserializeExistingNode( node, jsonNode[ Resources::DB::Fields::Base64Data ] );
}

void NodeManager::getZwaveNodeUpdateTimes( JsonObject& zwaveNodeUpdateTimes ) {
    forEachNode( [&zwaveNodeUpdateTimes]( const znet::ZNode& node ) {
        int32_t wakeUpInterval = node.getWakeUpInterval() ? *node.getWakeUpInterval() : 0;
        zwaveNodeUpdateTimes[ std::to_string( node.getId() ) ] = JSON(
            "last_update" << std::chrono::time_point_cast< std::chrono::seconds >( node.getLastUpdateTime() )
                                 .time_since_epoch()
                                 .count()
                          << "reachable" << ( node.isReachable() == znet::ReachableState::Reachable ? true : false )
                          << "is_wakeup" << node.isWakeup() << "wakeup_interval" << wakeUpInterval );
    } );
}

uint32_t NodeManager::getAlwaysOnDevicesInterval() const {
    return m_reachableCheckTimeoutSettings.getAlwaysOnDevicesInterval();
}

}  // namespace Zwave
}  // namespace HubPlatform
