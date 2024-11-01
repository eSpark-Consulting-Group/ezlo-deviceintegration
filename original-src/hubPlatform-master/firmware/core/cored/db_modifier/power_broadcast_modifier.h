#pragma once
#include "cored/db_modifier/broadcast_modifier.h"
#include <type_traits>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class PowerBroadcastModifier : public BroadcastModifier {
public:
    explicit PowerBroadcastModifier( IDbClient& dbClient );

    bool shouldUserBeNotified( const std::string& itemName,
                               const std::string& deviceCategory,
                               const std::string& deviceSubcategory,
                               const Json& broadcastData ) override;

    bool prepareUpdateBroadcast( const std::string& itemName,
                                 const std::string& deviceCategory,
                                 const std::string& deviceSubcategory,
                                 Json& broadcastData ) override;

    static void setUserNotification( Json& broadcastData, bool value );
    void setParentDeviceName( Json& broadcastData );

public:
    static bool powerSurge( const std::string& deviceCategory,
                            const std::string& deviceSubcategory,
                            const Json& broadcastData );
    static bool voltageDropDrift( const std::string& deviceCategory,
                                  const std::string& deviceSubcategory,
                                  const Json& broadcastData );
    static bool overCurrent( const std::string& deviceCategory,
                             const std::string& deviceSubcategory,
                             const Json& broadcastData );
    static bool overVoltageState( const std::string& deviceCategory,
                                  const std::string& deviceSubcategory,
                                  const Json& broadcastData );
    static bool overLoad( const std::string& deviceCategory,
                          const std::string& deviceSubcategory,
                          const Json& broadcastData );
    static bool loadError( const std::string& deviceCategory,
                           const std::string& deviceSubcategory,
                           const Json& broadcastData );
    static bool batteryMaintenance( const std::string& deviceCategory,
                                    const std::string& deviceSubcategory,
                                    const Json& broadcastData );
    static bool batteryCharging( const std::string& deviceCategory,
                                 const std::string& deviceSubcategory,
                                 const Json& broadcastData );
    static bool ac( const std::string& deviceCategory,
                    const std::string& deviceSubcategory,
                    const Json& broadcastData );

private:
    std::reference_wrapper< IDbClient > m_dbClientRef;
};

}  // namespace CoreD
}  // namespace HubPlatform
