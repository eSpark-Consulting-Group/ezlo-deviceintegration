#include "extensions_manager/extensions/plugins/gateway/gateway_plugin.h"
#include "extensions_manager/extensions/plugins/gateway/gateway_config.h"
#include "extensions_manager/em_environment.h"

#include "cored/remote_interface/cored_interface.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

#include <cassert>

namespace HubPlatform {
namespace ExtensionsManager {

GatewayPlugin::GatewayPlugin( Environment& environment,
                              std::unique_ptr< BaseConfig > baseConfigInfo,
                              std::unique_ptr< GatewayConfig > gatewayConfigInfo )
: Plugin( environment, std::move( baseConfigInfo ) )
, m_gatewayConfigInfo( std::move( gatewayConfigInfo ) ) {
    assert( m_gatewayConfigInfo );
}

void GatewayPlugin::init() {
    Plugin::init();

    using namespace CoreD::Model;

    Gateway gateway{ m_gatewayConfigInfo->m_name,
                     m_gatewayConfigInfo->m_unreachableReasons,
                     m_gatewayConfigInfo->m_unreachableActions,
                     getBaseConfig().summary.id,
                     m_gatewayConfigInfo->m_operations,
                     m_gatewayConfigInfo->m_label,
                     m_gatewayConfigInfo->m_reason,
                     m_gatewayConfigInfo->m_setItemValueCommand,
                     m_gatewayConfigInfo->m_setItemValueResponsePolicy,
                     m_gatewayConfigInfo->m_addItemDictionaryValueCommand,
                     m_gatewayConfigInfo->m_setItemDictionaryValueCommand,
                     m_gatewayConfigInfo->m_removeItemDictionaryValueCommand,
                     m_gatewayConfigInfo->m_clearItemDictionaryCommand,
                     m_gatewayConfigInfo->m_setGatewaySettingValueCommand,
                     m_gatewayConfigInfo->m_setSettingValueCommand,
                     m_gatewayConfigInfo->m_setSettingDictionaryValueCommand,
                     m_gatewayConfigInfo->m_resetSettingCommand,
                     m_gatewayConfigInfo->m_forceRemoveDeviceCommand,
                     m_gatewayConfigInfo->m_updateDeviceFirmwareCommand,
                     m_gatewayConfigInfo->m_checkDeviceCommand,
                     m_gatewayConfigInfo->m_createAbstractSubscriptionCommand,
                     m_gatewayConfigInfo->m_updateAbstractSubscriptionCommand,
                     m_gatewayConfigInfo->m_deleteAbstractSubscriptionCommand,
                     m_gatewayConfigInfo->m_dispatchAbstractSubscriptionCommand };

    auto result = getEnvironment().getCoreDInterface().ensureGateway( gateway );
    if( !result.second ) {
        /* TODO: such a case may happen only if we faced with bus/db denial */
        LOGGER_ERROR( cc::error( "GatewayPlugin: fail to init the 'gateway' plugin: fail to register the gateway" ) );
        return;
    }

    LOGGER_INFO( cc::info( "GatewayPlugin: the gateway (" ) + cc::note( m_gatewayConfigInfo->m_name ) +
                 cc::info( ") got registered for the plugin (" ) + cc::note( getBaseConfig().summary.toString() ) +
                 cc::info( ")" ) );
}

void GatewayPlugin::startup() {
    Plugin::startup();
    getEnvironment().getCoreDInterface().setGatewayStatus( getBaseConfig().summary.id,
                                                           CoreD::Model::GatewayStatus::Ready );
}

bool GatewayPlugin::migrate() {
    getEnvironment().getCoreDInterface().setGatewayStatus( getBaseConfig().summary.id,
                                                           CoreD::Model::GatewayStatus::Starting );
    return Plugin::migrate();
}

bool GatewayPlugin::teardown( const Json& operationInfo ) {
    getEnvironment().getCoreDInterface().setGatewayStatus( getBaseConfig().summary.id,
                                                           CoreD::Model::GatewayStatus::NotStarted );
    bool result = Plugin::teardown( operationInfo );
    return result;
}

bool GatewayPlugin::uninstall() {
    if( !Plugin::uninstall() ) {
        return false;
    }
    /* TODO: we have no idea if a plugin's teardown script finished by now so we may face with problems as we're
     *       unregistering gateway(s) which may be used by that script... */
    LOGGER_WARN( cc::warn( "GatewayPlugin: start to remove gateways, a plugin's teardown script may fail" ) );

    auto pluginId = getBaseConfig().summary.id;
    getEnvironment().getCoreDInterface().removePluginGateways( pluginId );

    LOGGER_INFO( cc::info( "GatewayPlugin: the gateway (" ) + cc::note( m_gatewayConfigInfo->m_name ) +
                 cc::info( ") got unregistered for the plugin (" ) + cc::note( getBaseConfig().summary.toString() ) +
                 cc::info( ")" ) );

    return true;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
