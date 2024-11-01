#pragma once

#include <znet/iznet_node_setting.hpp>

#include "common_utils/cpp14.h"
#include "json_utils/json_fwd.h"

#include <unordered_set>

class IDbClient;

namespace HubPlatform {
namespace Zwave {

class NodeSetting : public znet::INodeSetting {
public:
    static std::unique_ptr< NodeSetting > create( const Json& settings,
                                                  IDbClient& dbClient,
                                                  uint16_t manufacturerId,
                                                  uint16_t productType,
                                                  uint16_t productId );

    ~NodeSetting() override;

    void forEachConfiguration( std::function< void( const znet::Configuration& ) > callback ) const override;

    void forEachAssociation( std::function< void( const znet::Association& ) > callback ) const override;

    znet::utils::optional< uint32_t > getWakeUpInterval() const override;

    znet::utils::optional< bool > shouldRequestNotifications() const override;

    bool ignoreCcInterciew( uint8_t ccId ) const override;

    znet::utils::optional< bool > isExtendedUserCodesRequestRestricted() const override;

private:
    explicit NodeSetting( Json setting, Json databaseSetting );

private:
    const Json m_setting;
    const Json m_databaseSetting;

    std::unordered_set< uint8_t > m_ignoreCcInterview;
};

}  // namespace Zwave
}  // namespace HubPlatform
