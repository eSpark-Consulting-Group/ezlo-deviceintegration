#pragma once

#include "json_utils/json_fwd.h"

#include "cored/model/resources/cored_gateway_status.h"
#include "cored/model/resources/cored_manual_device_adding_capability.h"

#include <string>
#include <vector>
#include <utility>

namespace HubPlatform {

namespace UT {
class CoredRemoteInterfaceTestSuite;
}

namespace CoreDRemoteInterface {
class CoreDRemoteInterface;
}

namespace CoreD {
namespace Model {

/* @brief cored's representation of a gateway */
class Gateway final {
    friend class CoreDRemoteInterface::CoreDRemoteInterface;
    friend HubPlatform::UT::CoredRemoteInterfaceTestSuite;

public:
    using Reason = std::string;
    using Advice = std::string;
    using Label = std::string;
    using Method = std::string;

    using GatewayUnreachableReasons = std::vector< std::pair< Reason, Advice > >;
    using GatewayUnreachableActions = std::vector< std::pair< Label, Method > >;

    struct Operation {
        std::string m_name;
        std::string m_template;

        bool operator==( const Operation& other ) const {
            return m_name == other.m_name && m_template == other.m_template;
        }
    };

    using Operations = std::vector< Operation >;

    Gateway( std::string name,
             GatewayUnreachableReasons unreachableReasons,
             GatewayUnreachableActions unreachableActions,
             std::string pluginId, /* an id of a plugin this gateway is a part of */
             Operations operations = {},
             std::string label = {},
             std::string reason = {},
             std::string setItemValueCommand = {},
             std::string setItemValueResponsePolicy = {},
             std::string addItemDictionaryValueCommand = {},
             std::string setItemDictionaryValueCommand = {},
             std::string removeItemDictionaryValueCommand = {},
             std::string clearItemDictionaryCommand = {},
             std::string setGatewaySettingValueCommand = {},
             std::string setSettingValueCommand = {},
             std::string setSettingDictionaryValueCommand = {},
             std::string resetSettingCommand = {},
             std::string forceRemoveDeviceCommand = {},
             std::string updateDeviceFirmwareCommand = {},
             std::string checkDeviceCommand = {},
             std::string createAbstractSubscriptionCommand = {},
             std::string updateAbstractSubscriptionCommand = {},
             std::string deleteAbstractSubscriptionCommand = {},
             std::string dispatchAbstractSubscriptionCommand = {} );

    Gateway& setLabel( const std::string& label );
    Gateway& setReason( const std::string& reason );
    Gateway& setStatus( const GatewayStatus& status );

private:
    Json toJson() const;
    static void checkGatewayAsJson( const Json& gateway );

    JsonObject getButtonMethod( const std::string& label,
                                const std::string& description,
                                const std::string& method ) const;

private:
    const std::string m_name;     /* required (internal name of a gateway) */
    const std::string m_pluginId; /* required */

    const GatewayUnreachableReasons m_unreachableReasons; /* optional */
    const GatewayUnreachableActions m_unreachableActions; /* optional */
    const Operations m_operations;                        /* optional */
    std::string m_label;  /* optional (public name of a gateway), by default the same as a 'm_name' */
    std::string m_reason; /* optional, by default an empty string (public reason of not 'ready' status) */
    const std::string m_setItemValueCommand;                 /* optional, by default an empty string */
    const std::string m_setItemValueResponsePolicy;          /* optional, by default an empty string */
    const std::string m_addItemDictionaryValueCommand;       /* optional, by default an empty string */
    const std::string m_setItemDictionaryValueCommand;       /* optional, by default an empty string */
    const std::string m_removeItemDictionaryValueCommand;    /* optional, by default an empty string */
    const std::string m_clearItemDictionaryCommand;          /* optional, by default an empty string */
    const std::string m_setGatewaySettingValueCommand;       /* optional, by default an empty string */
    const std::string m_setSettingValueCommand;              /* optional, by default an empty string */
    const std::string m_setSettingDictionaryValueCommand;    /* optional, by default an empty string */
    const std::string m_resetSettingCommand;                 /* optional, by default an empty string */
    const std::string m_forceRemoveDeviceCommand;            /* optional, by default an empty string */
    const std::string m_updateDeviceFirmwareCommand;         /* optional, by default an empty string */
    const std::string m_checkDeviceCommand;                  /* optional, by default an empty string */
    const std::string m_createAbstractSubscriptionCommand;   /* optional, by default an empty string */
    const std::string m_updateAbstractSubscriptionCommand;   /* optional, by default an empty string */
    const std::string m_deleteAbstractSubscriptionCommand;   /* optional, by default an empty string */
    const std::string m_dispatchAbstractSubscriptionCommand; /* optional, by default an empty string */
    /* TODO: some settings, affecting whole gateway, may be provided within a plugin/gateway config file */
    const int m_settingsAmount = 0;

    /* std::string m_id;           hidden */

    const bool m_ready = true; /* hidden/removed, keep it as false (default value) to be compatible
                                  with previous hub API versions */

    GatewayStatus m_status;

    ManualDeviceAddingCapability m_manualDeviceAddingCapability;

    static const size_t m_minFieldsAmount = 11u;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
