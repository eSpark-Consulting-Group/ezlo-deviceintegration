#pragma once

#include <string>

#include "json_utils/json_fwd.h"

class FoUtils {
public:
    static bool checkIdIsItem( const std::string& objectId );
    static bool getItemTrigger( const std::string& itemId,
                                std::string& dataitemId,
                                std::string& method,
                                int& instance,
                                const std::string& blockType = "when" );
    static bool getSubdeviceId( const std::string& itemId,
                                std::string& dataitemId,
                                std::string& method,
                                int& instance,
                                const std::string& blockType = "when" );
    static std::string getDataItemId( const std::string& deviceId );
    static int getNodeIdByDeviceId( const std::string& deviceId );
    static std::string getDeviceIdByDataItemId( const std::string& dataitemId );
    static std::string getDeviceId( const std::string& itemId );
    static std::string getGatewayId( const std::string& deviceId );
    static std::string getGatewayAddress( const std::string& gatewayId );
    static std::string getItemId( const std::string& dataitemId, const std::string& method, const int instance );
    static bool getSubdevice( const std::string& itemId, int& nodeId, int& instanceId, std::string& method );
    static JsonObject getItemById( std::string itemId );
    static JsonObject getDeviceById( std::string deviceId );

    static void assignToDevices( const std::string& device_id,
                                 const std::string& user,
                                 const std::string& permission,
                                 JsonObject& answer );
    static void assignUserWithPermission( const std::string& db_name,
                                          const std::string& object_id,
                                          const std::string& user,
                                          const std::string& permission );
};
