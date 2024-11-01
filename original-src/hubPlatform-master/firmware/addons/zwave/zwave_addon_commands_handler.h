#pragma once

#include <functional>
#include <znet/iznet_provisioning_list_provider.hpp>
#include <znet/znet_provisioning_list_controller.hpp>

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace Zwave {

class ZwaveAddonCommandsHandler final {
public:
    ZwaveAddonCommandsHandler( znet::IProvisioningListProvider& provisioningListProvider,
                               znet::api::IProvisioningListController& provisioningListController );

    JsonObject handleAddonMessage( const JsonObject& request );

private:
    JsonObject handleProvisioningList( const JsonObject& params );
    JsonObject handleProvisioningAdd( const JsonObject& params );
    JsonObject handleProvisioningSet( const JsonObject& params );
    JsonObject handleProvisioningRemove( const JsonObject& params );

private:
    znet::IProvisioningListProvider& m_provisioningListProvider;
    znet::api::IProvisioningListController& m_provisioningListController;
};

}  // namespace Zwave
}  // namespace HubPlatform
