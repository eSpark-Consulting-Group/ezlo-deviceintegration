#include "cored/db_modifier/item_update_broadcast_protect_notification.h"
#include "cored/features_manager.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/values/value_factory.h"
#include "cored/db_modifier/security_item_db_modifier.h"
#include "json_utils/json_custom_getters.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/protect/protect_utils.h"
#include "cored/protect/security_config.h"
#include "cored/protect/resources.h"

namespace HubPlatform {
namespace CoreD {

ProtectNotificationCase::ProtectNotificationCase( Json& broadcastData,
                                                  const Json& device,
                                                  bool protectMode,
                                                  const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                                  const IFeaturesManager& featureManager,
                                                  SecurityItemDbModifier& securityItemDbModifier,
                                                  IDbClient& dbClient )
: m_broadcastData( broadcastData )
, m_device( device )
, m_protectMode( protectMode )
, m_bypassStatus( bypassStatus )
, m_featureManager( featureManager )
, m_dbClient( dbClient )
, m_securityItemDbModifier( securityItemDbModifier ) {
    m_securityItemDbModifier.prepareProtectServiceField( m_broadcastData, json_get_str( m_device, db::ID_FIELD_NAME ) );
}

ProtectNotificationDefaultCase::ProtectNotificationDefaultCase(
    Json& broadcastData,
    const Json& device,
    bool protectMode,
    const SecurityItemDbModifier::BypassStatus& bypassStatus,
    const IFeaturesManager& featureManager,
    SecurityItemDbModifier& securityItemDbModifier,
    IDbClient& dbClient )
: ProtectNotificationCase( broadcastData,
                           device,
                           protectMode,
                           bypassStatus,
                           featureManager,
                           securityItemDbModifier,
                           dbClient ) {}

void ProtectNotificationDefaultCase::process() {
    bool protectFeature = m_featureManager.getFeatureStatus( "protect" );
    if( protectFeature && m_securityItemDbModifier.isProtectNotification( m_broadcastData ) ) {
        processSecurityConfigFields();
    }
}

void ProtectNotificationDefaultCase::processSecurityConfigFields() {
    JsonObject result;
    const std::string securityConfigId = json_get_str( m_device, Model::Resources::DeviceFields::ProtectConfig );

    if( securityConfigId.empty() || securityConfigId == "default" ) {
        m_broadcastData[ "protectNotification" ] = true;
        return;
    }

    const auto securityConfigObject = Protect::ProtectUtils::getSecurityConfigObject( m_dbClient, securityConfigId );
    Protect::SecurityConfigInfo securityConfig;
    securityConfig.fromJson( securityConfigObject );

    if( !securityConfig.getAlarmFlag() ) {
        return;
    }

    m_broadcastData[ "protectNotification" ] = true;

    if( !securityConfig.getSoundType().empty() ) {
        result[ Protect::Resources::SecurityConfig::SoundType ] = securityConfig.getSoundType();
    }

    if( securityConfig.getChime() != "none" ) {
        result[ Protect::Resources::SecurityConfig::Chime ] = securityConfig.getChime();
    }

    if( !securityConfig.getSignalObject().empty() ) {
        securityConfig.buildSignalField(
            securityConfig.getSignalObject(), Protect::Resources::SignalFields::TrippedArmed, m_dbClient, result );
    }

    if( !result.empty() ) {
        m_broadcastData[ "protect" ] = result;
    }
}

ProtectNotificationShutdownFirstTimeCase::ProtectNotificationShutdownFirstTimeCase(
    Json& broadcastData,
    const Json& device,
    bool protectMode,
    const SecurityItemDbModifier::BypassStatus& bypassStatus,
    const IFeaturesManager& featureManager,
    SecurityItemDbModifier& securityItemDbModifier,
    IDbClient& dbClient )
: ProtectNotificationCase( broadcastData,
                           device,
                           protectMode,
                           bypassStatus,
                           featureManager,
                           securityItemDbModifier,
                           dbClient ) {}

void ProtectNotificationShutdownFirstTimeCase::process() {
    JsonObject protectField = JSON_EMPTY_OBJ;
    protectField[ "bypass" ] = "shutdown";
    protectField[ "cmsCode" ] = 575;
    m_broadcastData[ "protect" ] = protectField;
}

ProtectNotificationBypassCase::ProtectNotificationBypassCase( Json& broadcastData,
                                                              const Json& device,
                                                              bool protectMode,
                                                              const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                                              const IFeaturesManager& featureManager,
                                                              SecurityItemDbModifier& securityItemDbModifier,
                                                              IDbClient& dbClient )
: ProtectNotificationCase( broadcastData,
                           device,
                           protectMode,
                           bypassStatus,
                           featureManager,
                           securityItemDbModifier,
                           dbClient ) {}

void ProtectNotificationBypassCase::process() {
    JsonObject protectField = JSON_EMPTY_OBJ;
    protectField[ "bypass" ] = "user";
    m_broadcastData[ "protect" ] = protectField;
    addBypassSignal();
}

void ProtectNotificationBypassCase::addBypassSignal() {
    JsonObject result;
    const std::string securityConfigId = json_get_str( m_device, Model::Resources::DeviceFields::ProtectConfig );

    if( securityConfigId.empty() || securityConfigId == "default" ) {
        return;
    }

    const auto securityConfigObject = Protect::ProtectUtils::getSecurityConfigObject( m_dbClient, securityConfigId );
    Protect::SecurityConfigInfo securityConfig;
    securityConfig.fromJson( securityConfigObject );

    if( !securityConfig.getSignalObject().empty() ) {
        securityConfig.buildSignalField(
            securityConfig.getSignalObject(), Protect::Resources::SignalFields::Bypass, m_dbClient, result );
    }

    if( !result.empty() ) {
        m_broadcastData[ "protect" ] = result;
    }
}

ProtectNotificationShutdownFilteredCase::ProtectNotificationShutdownFilteredCase(
    Json& broadcastData,
    const Json& device,
    bool protectMode,
    const SecurityItemDbModifier::BypassStatus& bypassStatus,
    const IFeaturesManager& featureManager,
    SecurityItemDbModifier& securityItemDbModifier,
    IDbClient& dbClient )
: ProtectNotificationCase( broadcastData,
                           device,
                           protectMode,
                           bypassStatus,
                           featureManager,
                           securityItemDbModifier,
                           dbClient ) {}

void ProtectNotificationShutdownFilteredCase::process() {
    JsonObject protectField = JSON_EMPTY_OBJ;
    protectField[ "bypass" ] = "shutdown";
    m_broadcastData[ "protect" ] = protectField;
}

ProtectNotificationTestModeCase::ProtectNotificationTestModeCase(
    Json& broadcastData,
    const Json& device,
    bool protectMode,
    const SecurityItemDbModifier::BypassStatus& bypassStatus,
    const IFeaturesManager& featureManager,
    SecurityItemDbModifier& securityItemDbModifier,
    IDbClient& dbClient )
: ProtectNotificationCase( broadcastData,
                           device,
                           protectMode,
                           bypassStatus,
                           featureManager,
                           securityItemDbModifier,
                           dbClient ) {}

void ProtectNotificationTestModeCase::process() {
    m_broadcastData[ "protectNotification" ] = false;
}

}  // namespace CoreD
}  // namespace HubPlatform
