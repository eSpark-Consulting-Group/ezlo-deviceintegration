#pragma once
#include "json_utils/json_fwd.h"
#include <functional>

class IDbClient;

namespace HubPlatform {
namespace HardwareManager {

class BatteryInfo;

class BatteryInfoBroadcaster {
public:
    using BroadcastCallback = std::function< void( Json broadcast ) >;

public:
    explicit BatteryInfoBroadcaster( IDbClient& dbClient, BroadcastCallback broadcastCallback );
    virtual ~BatteryInfoBroadcaster();

    virtual void makeBroadcastAndSend( const BatteryInfo& info );

private:
    IDbClient& getDBClient();
    static std::pair< Json, bool > makeBroadcast( const BatteryInfo& info,
                                                  const Json& prevNotification,
                                                  const Json& diff );
    void updateNotificationPersistent( const Json& newInfo, bool userNotificationConditionChanged );
    void updateNotificationTemp( const Json& newInfo );
    Json fetchNotificationPersistent( const std::string& key );
    Json fetchNotificationTemp( const std::string& key );

private:
    IDbClient& m_dbClient;
    BroadcastCallback m_broadcastCallback;
    bool m_savedToPersistentDb{ false };
};

}  // namespace HardwareManager
}  // namespace HubPlatform
