#pragma once

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include <string>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdownDeviceStatistic {
public:
    explicit SwingerShutdownDeviceStatistic( const Json& record );

public:
    std::string getId() const;
    int getHits() const;
    int64_t getAdded() const;
    int64_t getUpdated() const;
    bool getDeleted() const;

    void setId( const std::string& id );
    void setHits( int hits );
    void setAdded( int64_t added );
    void setUpdated( int64_t updated );
    void setDeleted( bool deleted );
    Json toJson() const;
    bool fromJson( const JsonObject& data );

    static bool isSupportedDBName( const std::string& dbName );
    static bool createDbRecord( const std::string& id,
                                JsonObject query,
                                const std::string& dbName,
                                IDbClient& dbClient );
    static bool updateDbRecord( const std::string& id,
                                JsonObject updateObj,
                                const std::string& dbName,
                                IDbClient& dbClient );
    static Json loadFromDb( const std::string& id, const std::string& dbName, IDbClient& dbClient );
    static bool deleteDbRecord( const std::string& id, const std::string& dbName, IDbClient& dbClient );
    static JsonArray loadAllFromDb( const std::string& dbName,
                                    JsonObject& request,
                                    const Json* fieldsToReturn,
                                    IDbClient& dbClient );

private:
    std::string m_id;
    int m_hits = 0;
    int64_t m_added = 0;
    int64_t m_updated = 0;
    bool m_deleted{ false };
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform