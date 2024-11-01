#pragma once

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include <string>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdownTimeSeries {
public:
    SwingerShutdownTimeSeries( Json& record );

public:
    std::string getId() const;
    std::string getDeviceId() const;
    int getHits() const;
    int64_t getBegin() const;
    int64_t getUpdated() const;
    bool getDeleted() const;

    void setId( const std::string& id );
    void setDeviceId( const std::string& id );
    void setHits( int hits );
    void setBegin( int64_t added );
    void setUpdated( int64_t updated );
    void setDeleted( bool deleted );
    Json toJson() const;
    bool fromJson( const JsonObject& data );

    static bool isSupportedDBName( const std::string& dbName );
    static bool createDbRecord( std::string id, JsonObject query, const std::string& dbName, IDbClient& dbClient );
    static bool updateDbRecord( std::string id, JsonObject updateObj, const std::string& dbName, IDbClient& dbClient );
    static Json loadFromDb( std::string id, const std::string& dbName, IDbClient& dbClient );
    static JsonArray loadRecordsFromDbWithDeviceId( std::string id, const std::string& dbName, IDbClient& dbClient );
    static bool deleteDbRecord( std::string id, const std::string& dbName, IDbClient& dbClient );
    static JsonArray loadAllRecordsFromDb( const std::string& dbName, const JsonObject& object, IDbClient& dbClient );

private:
    std::string m_id;
    std::string m_deviceId;
    int m_hits = 0;
    int64_t m_begin = 0;
    int64_t m_updated = 0;
    bool m_deleted{ false };
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform