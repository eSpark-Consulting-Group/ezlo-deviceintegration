#include "extensions_manager/extensions/plugins/gateway/gateway_config.h"
#include "cored/model/resources/cored_resources.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_custom_getters.h"
#include "common_utils/cpp14.h"

#include <stdexcept>

namespace HubPlatform {
namespace ExtensionsManager {

namespace {
/* a section of data within a plugin's config specific to 'gateway' */
constexpr const char DataSectionName[] = "gateway";
}  // namespace

std::unique_ptr< GatewayConfig > GatewayConfig::getGatewayConfig( const Json& joConfig ) {
    using namespace JsonCustomGetters;
    using StringChecking = CheckJsonStringStrictPolicy;

    using UnreachableReason = GatewayUnreachableReasons::value_type;
    auto unreachableReasonsGetter = makeVectorGetter(
        makeStructGetter( makeMemberMapping( &UnreachableReason::first, "reason", StringChecking() ),
                          makeMemberMapping( &UnreachableReason::second, "advice", StringChecking() ) ) );

    using UnreachableAction = GatewayUnreachableActions::value_type;
    auto unreachableActionsGetter = makeVectorGetter(
        makeStructGetter( makeMemberMapping( &UnreachableAction::first, "label", StringChecking() ),
                          makeMemberMapping( &UnreachableAction::second, "method", StringChecking() ) ) );

    auto operationsGetter =
        makeVectorGetter( makeStructGetter( makeMemberMapping( &Operation::m_template, "template" ),
                                            makeMemberMapping( &Operation::m_name, "operation" ) ) );

    using namespace CoreD::Model::Resources;
    const auto configGetter = makeFieldGetter(
        DataSectionName,
        makeStructGetter(
            makeMemberMapping( &GatewayConfig::m_name, "name", StringChecking() ),
            makeMemberMapping( &GatewayConfig::m_label, "label", StringChecking() ),
            makeMemberMapping( &GatewayConfig::m_reason, "reason", OptionalMemberPolicy() ),
            makeMemberMapping(
                &GatewayConfig::m_setItemValueCommand, GatewayFields::SetItemValueCommandFieldName, StringChecking() ),
            makeMemberMapping( &GatewayConfig::m_setItemValueResponsePolicy,
                               GatewayFields::SetItemValueResponsePolicyFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_addItemDictionaryValueCommand,
                               GatewayFields::AddItemDictionaryValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_setItemDictionaryValueCommand,
                               GatewayFields::SetItemDictionaryValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_removeItemDictionaryValueCommand,
                               GatewayFields::RemoveItemDictionaryValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_clearItemDictionaryCommand,
                               GatewayFields::ClearItemDictionaryCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_setGatewaySettingValueCommand,
                               GatewayFields::SetGatewaySettingValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_setSettingValueCommand,
                               GatewayFields::SetSettingValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_setSettingDictionaryValueCommand,
                               GatewayFields::SetSettingDictionaryValueCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_resetSettingCommand,
                               GatewayFields::ResetSettingCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_forceRemoveDeviceCommand,
                               GatewayFields::ForceRemoveDeviceCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_updateDeviceFirmwareCommand,
                               GatewayFields::UpdateDeviceFirmwareCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_checkDeviceCommand,
                               GatewayFields::CheckDeviceCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_unreachableReasons,
                               makeFieldGetter( GatewayFields::UnreachableReasonsFieldName,
                                                std::move( unreachableReasonsGetter ),
                                                OptionalMemberPolicy() ) ),
            makeMemberMapping( &GatewayConfig::m_unreachableActions,
                               makeFieldGetter( GatewayFields::UnreachableActionsFieldName,
                                                std::move( unreachableActionsGetter ),
                                                OptionalMemberPolicy() ) ),
            makeMemberMapping( &GatewayConfig::m_createAbstractSubscriptionCommand,
                               GatewayFields::CreateAbstractSubscriptionCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_updateAbstractSubscriptionCommand,
                               GatewayFields::UpdateAbstractSubscriptionCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_deleteAbstractSubscriptionCommand,
                               GatewayFields::DeleteAbstractSubscriptionCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping( &GatewayConfig::m_dispatchAbstractSubscriptionCommand,
                               GatewayFields::DispatchAbstractSubscriptionCommandFieldName,
                               OptionalMemberPolicy() ),
            makeMemberMapping(
                &GatewayConfig::m_operations,
                makeFieldGetter( "operations", std::move( operationsGetter ), OptionalMemberPolicy() ) ) ) );

    auto result = std::make_unique< GatewayConfig >();
    if( !configGetter( joConfig, *result ) ) {
        Log::error( cc::error( "Error during loading gateway config from json" ) );
        return nullptr;
    }

    return result;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
