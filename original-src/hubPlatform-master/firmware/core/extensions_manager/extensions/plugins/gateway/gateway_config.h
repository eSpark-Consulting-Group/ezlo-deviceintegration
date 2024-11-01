#pragma once

#include "cored/model/cored_gateway.h"

#include "json_utils/json_fwd.h"

#include <memory>
#include <string>

namespace HubPlatform {
namespace ExtensionsManager {

struct GatewayConfig final {
    using GatewayUnreachableReasons = HubPlatform::CoreD::Model::Gateway::GatewayUnreachableReasons;
    using GatewayUnreachableActions = HubPlatform::CoreD::Model::Gateway::GatewayUnreachableActions;
    using Operation = HubPlatform::CoreD::Model::Gateway::Operation;
    using Operations = HubPlatform::CoreD::Model::Gateway::Operations;

    static std::unique_ptr< GatewayConfig > getGatewayConfig( const Json& joConfig );

    std::string m_name;
    std::string m_label;
    std::string m_reason;
    std::string m_setItemValueCommand;
    std::string m_setItemValueResponsePolicy;
    std::string m_addItemDictionaryValueCommand;
    std::string m_setItemDictionaryValueCommand;
    std::string m_removeItemDictionaryValueCommand;
    std::string m_clearItemDictionaryCommand;
    std::string m_setGatewaySettingValueCommand;
    std::string m_setSettingValueCommand;
    std::string m_setSettingDictionaryValueCommand;
    std::string m_resetSettingCommand;
    std::string m_forceRemoveDeviceCommand;
    std::string m_updateDeviceFirmwareCommand;
    std::string m_checkDeviceCommand;
    std::string m_createAbstractSubscriptionCommand;
    std::string m_updateAbstractSubscriptionCommand;
    std::string m_deleteAbstractSubscriptionCommand;
    std::string m_dispatchAbstractSubscriptionCommand;

    GatewayUnreachableReasons m_unreachableReasons;
    GatewayUnreachableActions m_unreachableActions;

    Operations m_operations;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
