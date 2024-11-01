#pragma once

#include "base_utils/non_copyable.h"
#include "json_utils/json_fwd.h"

#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"

#include <map>
#include <memory>

class IDbClient;

namespace znet {
class ZNode;
class RequestsManager;
class IAssociationsConfigurator;
class ReachableCheckTimeoutSettings;
}  // namespace znet

namespace HubPlatform {
namespace Zwave {

class NodeManager final : private common::NonCopyable {
public:
    explicit NodeManager( IDbClient& dbClient,
                          znet::RequestsManager& requestManager,
                          znet::IAssociationsConfigurator& associationsConfigurator,
                          znet::ReachableCheckTimeoutSettings& reachableCheckTimeoutSettings );
    ~NodeManager();

    znet::ZNode* findNode( uint8_t nodeId );
    const znet::ZNode* findNode( uint8_t nodeId ) const;

    void resetNodeCache( uint8_t nodeId );

    znet::ZNode& registerNode( uint8_t nodeId );
    bool remove( uint8_t nodeId );
    bool update( uint8_t nodeId );

    static Json serializeNode( const znet::ZNode& node );
    static std::unique_ptr< znet::ZNode > deserializeNode(
        const Json& jsonNode,
        znet::RequestsManager* requestManager,
        znet::IAssociationsConfigurator* associationsConfigurator,
        znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings );

    static void deserializeExistionNode( znet::ZNode& node, const Json& jsonNode );

    void reset();

    template < typename Handler >
    void forEachNode( Handler&& handler ) const {
        yamc::shared_lock< yamc::alternate::shared_mutex > sharedLocker( m_cacheMutex );
        for( const auto& idToNode : m_cachedNodes ) {
            handler( const_cast< const znet::ZNode& >( ( *idToNode.second ) ) );
        }
    }
    void getZwaveNodeUpdateTimes( JsonObject& zwaveNodeUpdateTimes );

    uint32_t getAlwaysOnDevicesInterval() const;

private:
    template < typename Manager >
    static znet::ZNode* findNodeImpl( Manager& nodeManager, uint8_t nodeId );

private:
    IDbClient& m_dbClient;
    znet::RequestsManager& m_requestManager;
    znet::IAssociationsConfigurator& m_associationsConfigurator;
    znet::ReachableCheckTimeoutSettings& m_reachableCheckTimeoutSettings;

    mutable yamc::alternate::shared_mutex m_cacheMutex;
    mutable std::map< uint8_t, std::unique_ptr< znet::ZNode > > m_cachedNodes;
};

}  // namespace Zwave
}  // namespace HubPlatform
