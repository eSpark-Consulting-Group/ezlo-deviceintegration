#include "cored/db_modifier/security_item_db_modifier.h"
#include "cored/features_manager.h"
#include "cored/house_modes/house_mode_utils.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/values/value_factory.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "service_interfaces/service.h"
#include <stdexcept>

namespace HubPlatform {
namespace CoreD {

SecurityItemDbModifier::SecurityItemDbModifier( Services::Service& service,
                                                const IFeaturesManager& featureManager,
                                                SwingerShutdown::SwingerShutdown& swingerShutdown )
: m_service( service )
, m_featureManager( featureManager )
, m_swingerShutdown( swingerShutdown ) {}

bool SecurityItemDbModifier::isProtectNotification( const Json& updateNotification ) {
    const std::string name = json_get_str( updateNotification, "name" );
    const std::string value = json_get_str( updateNotification, "value" );
    const std::string deviceCategory = json_get_str( updateNotification, "deviceCategory" );
    const std::string deviceSubcategory = json_get_str( updateNotification, "deviceSubcategory" );
    const bool deviceArmed = json_get_bool( updateNotification, "deviceArmed", false );
    const auto currentHouseMode = CoreDHouseModes::getCurrentHouseMode( m_service.getDBClient() );
    const auto protectMode = currentHouseMode->m_protect;

    if( ( protectMode && name == "security_threat" && deviceCategory == "security_sensor" &&
          ( deviceSubcategory == "door" || deviceSubcategory == "motion" ) ) &&
        deviceArmed ) {
        return true;
    }

    else if( ( name == "security_threat" && json_get_bool( updateNotification, "value", false ) &&
               deviceCategory == "scene_controller" && deviceSubcategory == "panic" &&
               !json_get_str( updateNotification, "protectService" ).empty() ) &&
             deviceArmed ) {
        return true;
    }

    else if( ( name == "co_alarm" && ( value == "co_detected" || value == "no_co" ) ) ) {
        return true;
    }
    else if( ( name == "heat_alarm" &&
               ( value == "heat_ok" || value == "overheat_detected" || value == "under_heat_detected" ) ) ) {
        return true;
    }
    else if( ( name == "glass_breakage_alarm" && ( value == "glass_breakage" || value == "no_glass_breakage" ) ) ) {
        return true;
    }
    else if( ( name == "gas_alarm" &&
               ( value == "no_gas" || value == "combustible_gas_detected" || value == "toxic_gas_detected" ) ) ) {
        return true;
    }
    else if( ( name == "intrusion_alarm" && ( value == "no_intrusion" || value == "intrusion" ) ) ) {
        return true;
    }
    else if( ( name == "tampering_cover_alarm" && ( value == "no_tampering_cover" || value == "tampering_cover" ) ) ) {
        return true;
    }
    else if( ( name == "tampering_move_alarm" && ( value == "no_moving" || value == "product_moved" ) ) ) {
        return true;
    }
    else if( ( name == "tampering_impact_alarm" && ( value == "no_impact" || value == "impact_detected" ) ) ) {
        return true;
    }
    else if( ( name == "tampering_invalid_code_alarm" && ( value == "no_invalid_code" || value == "invalid_code" ) ) ) {
        return true;
    }
    else if( ( name == "tilt_alarm" && ( value == "no_tilt" || value == "tilt" ) ) ) {
        return true;
    }
    else if( ( name == "protect_alarm" &&
               ( value == "alarm_active" || value == "alarm_active_silent" || value == "alarm_inactive" ) ) ) {
        return true;
    }
    else if( ( name == "smoke_alarm" && ( value == "smoke_detected" || value == "no_smoke" ) ) ) {
        return true;
    }
    else if( protectMode && name == "battery" && json_get_int( updateNotification, "value" ) <= 20 &&
             deviceCategory == "security_sensor" ) {
        return true;
    }
    else if( protectMode && json_get_bool( updateNotification, "userNotification", false ) ) {
        auto deviceId = json_get_str( updateNotification, "deviceId" );
        auto dbFilter = JSON( db::ID_FIELD_NAME << deviceId );
        auto foundIds = CoreDHouseModes::getProtectDevicesIds( m_service.getDBClient(), dbFilter );
        if( !foundIds.empty() )
            return true;
    }

    return false;
}

bool SecurityItemDbModifier::prepareProtectServiceField( Json& broadcast, const std::string& deviceId ) {
    if( ( !deviceId.empty() && json_get_bool( broadcast, "deviceArmed", false ) &&
          json_get_str( broadcast, "deviceSubcategory" ) == "panic" &&
          json_get_str( broadcast, "deviceCategory" ) == "scene_controller" &&
          json_get_str( broadcast, "name" ) == "security_threat" ) ||
        ( !deviceId.empty() && json_get_str( broadcast, "deviceCategory" ) == "sos_button" &&
          json_get_str( broadcast, "name" ) == "protect_alarm" ) ) {
        auto findResult = CoreDHouseModes::findProtectButtonById( m_service.getDBClient(), deviceId );
        if( findResult.second ) {
            json_add_str( broadcast, "protectService", CoreDHouseModes::toString( findResult.first.service() ) );
            return true;
        }
    }
    return false;
}

SecurityItemDbModifier::BypassStatus SecurityItemDbModifier::getBypassModeProtectNotification(
    const CoreDHouseModes::HouseModeEntry& currentHouseMode,
    const std::string& deviceId ) {
    const bool bypassDevice = currentHouseMode.m_bypassDeviceIds.count( deviceId ) > 0;
    const bool protect = currentHouseMode.m_protect;
    if( protect && m_featureManager.getFeatureStatus( "protect" ) ) {
        if( bypassDevice )
            return SecurityItemDbModifier::BypassStatus::BYPASS;
        else if( filteredBySwingerShutdownIfEnabledByFeatures() ) {
            m_swingerShutdown.addHit( deviceId );
            return filteredBySwingerShutdown( deviceId );
        }
    }

    return SecurityItemDbModifier::BypassStatus::NONE;
}

bool SecurityItemDbModifier::filteredBySwingerShutdownIfEnabledByFeatures() {
    return m_featureManager.getFeatureStatus( "protect.swinger_shutdown" );
}

SecurityItemDbModifier::BypassStatus SecurityItemDbModifier::filteredBySwingerShutdown( const std::string& deviceId ) {
    bool isSwingerShutdownDevice = m_swingerShutdown.isExist( deviceId );
    bool disabled = m_swingerShutdown.isDisabled( deviceId );
    if( isSwingerShutdownDevice && !disabled ) {
        bool reached = m_swingerShutdown.isReachedDeviceHitsLimit( deviceId );
        if( !reached ) {
            return getUnreachedStatus( deviceId );
        }
    }
    return BypassStatus::SHUTDOWN_PASS;
}

SecurityItemDbModifier::BypassStatus SecurityItemDbModifier::getUnreachedStatus( const std::string& deviceId ) {
    if( m_swingerShutdown.getHits( deviceId ) == 1 ) {
        return BypassStatus::SHUTDOWN_FIRST_TIME;
    }
    return BypassStatus::SHUTDOWN_FILTERED;
}

void SecurityItemDbModifier::setTestMode( bool testMode ) {
    m_testMode = testMode;
}

bool SecurityItemDbModifier::inTestMode() {
    return m_testMode;
}

}  // namespace CoreD
}  // namespace HubPlatform
