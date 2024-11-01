#pragma once

#include "json_utils/json_utils.h"
#include <functional>

namespace HubPlatform {
namespace HardwareManager {

enum class BatteryStatus { Initializing, Missing, Charging, Full, Discharging, Drained };

struct BatteryInfo {
    static constexpr int CriticalStateOfChargeLevel = 10;
    static constexpr int LowStateOfChargeLevel = 25;

    BatteryStatus status;
    int stateOfCharge;
    int remainingTime;
    int health;
    bool depleted;

    BatteryInfo()
    : status( BatteryStatus::Initializing )
    , stateOfCharge( 0 )
    , remainingTime( 0 )
    , health( 0 )
    , depleted( false ) {}

    bool isValid() const {
        switch( status ) {
        case BatteryStatus::Initializing:
        case BatteryStatus::Missing:
            return false;
        default:
            return true;
        }
    }

    bool isDischarging() const {
        return status == BatteryStatus::Discharging;
    }

    bool isLow() const {
        if( !isValid() )
            return false;

        return stateOfCharge <= LowStateOfChargeLevel;
    }

    bool isCritical() const {
        if( !isValid() )
            return false;

        return stateOfCharge <= CriticalStateOfChargeLevel;
    }

    bool isDrained() const {
        return status == BatteryStatus::Drained;
    }

    Json toJson() const {
        Json result;
        result[ "status" ] = statusToString( status );
        result[ "stateOfCharge" ] = stateOfCharge;
        result[ "remainingTime" ] = remainingTime;
        result[ "health" ] = health;
        result[ "depleted" ] = depleted;
        return result;
    }

    std::string toString() const {
        std::string result;
        if( status == BatteryStatus::Initializing || status == BatteryStatus::Missing ) {
            result = statusToString( status );
            return result;
        }

        result = std::to_string( stateOfCharge ) + "%, " + statusToString( status );

        if( status != BatteryStatus::Full ) {
            int hours = remainingTime / 60;
            int minutes = remainingTime % 60;
            result += ", remaining time: ";
            if( hours > 0 )
                result += std::to_string( hours ) + " h ";
            result += std::to_string( minutes ) + " min";
        }
        if( depleted )
            result += " [WARNING: Depleted!]";
        return result;
    };

    static BatteryInfo fromJson( const Json& json ) {
        BatteryInfo info;
        info.status = statusFromString( json_get_str( json, "status" ) );
        info.stateOfCharge = json_get_int( json, "stateOfCharge", 0 );
        info.remainingTime = json_get_int( json, "remainingTime", 0 );
        info.health = json_get_int( json, "health", 0 );
        info.depleted = json_get_bool( json, "depleted", false );
        return info;
    };

    static std::string statusToString( BatteryStatus status ) {
        switch( status ) {
        case BatteryStatus::Initializing:
            return "initializing";
        case BatteryStatus::Missing:
            return "missing";
        case BatteryStatus::Charging:
            return "charging";
        case BatteryStatus::Full:
            return "full";
        case BatteryStatus::Discharging:
            return "discharging";
        case BatteryStatus::Drained:
            return "drained";
        }
        return {};
    };

    static BatteryStatus statusFromString( const std::string& status ) {
        if( status == "initializing" ) {
            return BatteryStatus::Initializing;
        }
        else if( status == "missing" ) {
            return BatteryStatus::Missing;
        }
        else if( status == "charging" ) {
            return BatteryStatus::Charging;
        }
        else if( status == "full" ) {
            return BatteryStatus::Full;
        }
        else if( status == "discharging" ) {
            return BatteryStatus::Discharging;
        }
        else if( status == "drained" ) {
            return BatteryStatus::Drained;
        }
        return BatteryStatus::Initializing;
    };
};

using BatteryInfoChangedCallback = std::function< void( const BatteryInfo& ) >;

}  // namespace HardwareManager
}  // namespace HubPlatform
