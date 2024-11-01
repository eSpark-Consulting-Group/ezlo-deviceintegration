#include "hardware_manager/battery/battery_info_broadcaster.h"
#include "hardware_manager/battery/battery_info.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "database_client/fake_db/fake_db_client.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace HardwareManager {

namespace {
const char* const BatteryNotificationId = "notification";
}  // namespace

BatteryInfoBroadcaster::BatteryInfoBroadcaster( IDbClient& dbClient, BroadcastCallback broadcastCallback )
: m_dbClient( dbClient )
, m_broadcastCallback( std::move( broadcastCallback ) ) {}

BatteryInfoBroadcaster::~BatteryInfoBroadcaster() = default;

IDbClient& BatteryInfoBroadcaster::getDBClient() {
    return m_dbClient;
}

void BatteryInfoBroadcaster::makeBroadcastAndSend( const BatteryInfo& info ) {
    auto newInfo = info.toJson();
    newInfo[ db::ID_FIELD_NAME ] = BatteryNotificationId;
    Json prevTempNotification = fetchNotificationTemp( BatteryNotificationId );
    Json prevPersistentNotification = fetchNotificationPersistent( BatteryNotificationId );

    auto& prevNotification = prevTempNotification.empty() ? prevPersistentNotification : prevTempNotification;
    auto diff = jsonDiff( prevNotification, newInfo );
    auto currentCheck = std::chrono::steady_clock::now();
    static auto prevCheck = currentCheck;

    bool userNotificationConditionChanged = false;
    if( !diff.empty() &&
        ( diff.count( "remainingTime" ) != diff.size() || ( currentCheck - prevCheck > std::chrono::minutes( 1 ) ) ) ) {
        updateNotificationTemp( newInfo );
        LOGGER_DEBUG_RAW( cc::debug( "Battery: " ) + cc::note( info.toString() ) );
        if( !prevNotification.empty() ) {
            Json broadcast;
            std::tie( broadcast, userNotificationConditionChanged ) = makeBroadcast( info, prevNotification, diff );

            if( m_broadcastCallback != nullptr )
                m_broadcastCallback( broadcast );
            prevCheck = currentCheck;
        }
    }
    updateNotificationPersistent( newInfo, userNotificationConditionChanged );
}

std::pair< Json, bool > BatteryInfoBroadcaster::makeBroadcast( const BatteryInfo& info,
                                                               const Json& prevNotification,
                                                               const Json& diff ) {
    bool userNotificationConditionChanged = false;
    Json broadcast;
    broadcast[ "battery" ] = diff;

    bool userNotification = false;
    bool prevLow = json_get_int( prevNotification, "stateOfCharge", 100 ) <= BatteryInfo::LowStateOfChargeLevel;
    bool newLow = info.stateOfCharge <= BatteryInfo::LowStateOfChargeLevel;
    const auto prevStatus = BatteryInfo::statusFromString( json_get_str( prevNotification, "status" ) );
    bool prevDischarging = prevStatus == BatteryStatus::Discharging;
    bool newDischarging = info.status == BatteryStatus::Discharging;
    const auto isConnectedToPower = prevDischarging && !newDischarging;

    if( ( prevLow != newLow ) || ( prevDischarging != newDischarging ) ) {
        userNotificationConditionChanged = true;
    }
    if( !prevLow && newLow ) {
        userNotification = true;
    }
    if( !prevDischarging && newDischarging ) {
        broadcast[ "battery" ][ "stateOfCharge" ] = info.stateOfCharge;
        userNotification = true;
    }
    if( isConnectedToPower ) {
        userNotification = true;
    }
    if( userNotification ) {
        broadcast[ "userNotification" ] = true;
    }
    return { broadcast, userNotificationConditionChanged };
}

void BatteryInfoBroadcaster::updateNotificationTemp( const Json& newInfo ) {
    getDBClient().update( TempBatteryInfoDBName, JSON_EMPTY_OBJ, newInfo, true );
}

Json BatteryInfoBroadcaster::fetchNotificationPersistent( const std::string& key ) {
    auto prevPersistentNotification =
        getDBClient().findOne( BatteryInfoDBName, JSON( db::ID_FIELD_NAME << key ), nullptr );
    return prevPersistentNotification;
}

Json BatteryInfoBroadcaster::fetchNotificationTemp( const std::string& key ) {
    auto prevTempNotification =
        getDBClient().findOne( TempBatteryInfoDBName, JSON( db::ID_FIELD_NAME << key ), nullptr );
    return prevTempNotification;
}

void BatteryInfoBroadcaster::updateNotificationPersistent( const Json& newInfo,
                                                           bool userNotificationConditionChanged ) {
    if( !m_savedToPersistentDb || userNotificationConditionChanged ) {
        getDBClient().update( BatteryInfoDBName, JSON_EMPTY_OBJ, newInfo, true );
        m_savedToPersistentDb = true;
    }
}

}  // namespace HardwareManager
}  // namespace HubPlatform