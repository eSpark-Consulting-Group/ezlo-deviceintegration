#pragma once

#include "external_controllers/ec_messenger.h"
#include "database_client/idbclient.h"

namespace HubPlatform {

namespace ExternalControllers {

class Core final {
public:
    Core( Messenger& messenger, IDbClient& dbClient );

    void subscribe( const std::string& controllerId,
                    std::vector< std::string > itemsIds,
                    std::vector< std::string > devicesIds );

    void unsubscribe( const std::string& controllerId,
                      std::vector< std::string > itemsIds,
                      std::vector< std::string > devicesIds );

    void syncControllerData( const std::string& controllerId );

    void updateItemsList( const std::string& controllerId, const JsonArray& items );
    void updateDevicesList( const std::string& controllerId, const JsonArray& devices );

    JsonObject getItem( const std::string& controllerId, const std::string& deviceId ) const;
    bool itemSubscribed( const std::string& controllerId, const std::string& deviceId ) const;
    bool itemExists( const std::string& controllerId, const std::string& deviceId ) const;
    void addItem( const std::string& controllerId, const std::string& itemId, Json item );
    void updateItem( const std::string& controllerId, const std::string& itemId, const Json& item );
    void removeItem( const std::string& controllerId, const std::string& itemId );

    bool deviceSubscribed( const std::string& controllerId, const std::string& deviceId ) const;
    bool deviceExists( const std::string& controllerId, const std::string& deviceId ) const;
    void addDevice( const std::string& controllerId, const std::string& deviceId, Json device );
    void updateDevice( const std::string& controllerId, const std::string& deviceId, const Json& device );
    void removeDevice( const std::string& controllerId, const std::string& deviceId );

private:
    Json getItemDiff( const std::string& controllerId, const std::string& itemId, const Json& newItem ) const;
    Json getDeviceDiff( const std::string& controllerId, const std::string& deviceId, const Json& newDevice ) const;

private:
    Messenger& m_messenger;
    IDbClient& m_dbClient;
};

}  // namespace ExternalControllers
}  // namespace HubPlatform
