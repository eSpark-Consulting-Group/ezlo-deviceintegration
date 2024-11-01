#include "http/plugin_connection_queue.h"

namespace HubPlatform {
namespace CommonHttp {

bool PluginConnectionQueue::askAddToMultiHandle( const PluginId& pluginId, const ConnectionId& connId ) {
    UniqueLock lock( m_queueMutex );

    m_queueToAdd.emplace_front( std::make_pair( pluginId, connId ) );
    return true;
}

void PluginConnectionQueue::pushBackToAdd( std::pair< PluginId, ConnectionId > idPair ) {
    UniqueLock lock( m_queueMutex );
    m_queueToAdd.insert( m_queueToAdd.end(), std::move( idPair ) );
}

void PluginConnectionQueue::pushBackToRemove( std::pair< PluginId, ConnectionId > idPair ) {
    UniqueLock lock( m_queueMutex );
    m_queueToRemove.insert( m_queueToAdd.end(), std::move( idPair ) );
}

bool PluginConnectionQueue::askRemoveFromMultiHandle( const PluginId& pluginId, const ConnectionId& connId ) {
    UniqueLock lock( m_queueMutex );

    auto predicate = [&pluginId, &connId]( const ConnectionIdPair& pluginConnection ) {
        return pluginConnection.first == pluginId && pluginConnection.second == connId;
    };

    auto foundIt = std::find_if( begin( m_queueToAdd ), end( m_queueToAdd ), predicate );
    if( foundIt != end( m_queueToAdd ) ) {
        foundIt = m_queueToAdd.erase( foundIt );
    }
    else {
        m_queueToRemove.emplace_front( std::make_pair( pluginId, connId ) );
    }
    return true;
}

bool PluginConnectionQueue::markForRemoval( ConnectionPtr connection ) {
    UniqueLock lock( m_queueMutex );

    m_markedForRemoval.insert( make_pair( connection->getId(), std::move( connection ) ) );
    return true;
}

void PluginConnectionQueue::extractCollections( ConnectionIdPairList& copyToAdd,
                                                ConnectionIdPairList& copyToRemove,
                                                ConnectionMap& copyMarkedForRemoval ) {
    UniqueLock lock( m_queueMutex );

    copyToAdd.swap( m_queueToAdd );
    copyToRemove.swap( m_queueToRemove );
    copyMarkedForRemoval.swap( m_markedForRemoval );
}

}  // namespace CommonHttp
}  // namespace HubPlatform