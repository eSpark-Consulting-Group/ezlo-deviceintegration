#pragma once
#include "database_client/idbclient.h"
#include <json_utils/json_utils.h>
#include <chrono>

namespace HubPlatform {
namespace Uid {
namespace Reporter {

class ConnectionReportDbInfo {
public:
    explicit ConnectionReportDbInfo( const Json& record );

public:
    std::string getId() const;
    std::chrono::seconds getUptime() const;
    int getReconnects() const;

    void setId( const std::string& id );
    void setUptime( std::chrono::seconds timestamp );
    void setReconnects( int reconnected );
    Json toJson() const;
    bool fromJson( const JsonObject& data );

    static void createDbRecord( JsonObject query, const std::string dbName, IDbClient& dbClient );
    static void updateDbRecord( JsonObject updateObj, const std::string& dbName, IDbClient& dbClient );
    static Json loadFromDb( const std::string& dbName, IDbClient& dbClient );
    static void deleteDbRecord( const std::string& id, const std::string& dbName, IDbClient& dbClient );

private:
    std::string m_id;
    std::chrono::seconds m_uptime{ 0 };
    int m_reconnects = 0;
    std::chrono::seconds m_lastMonotonicConnected{ 0 };
};

}  // namespace Reporter
}  // namespace Uid
}  // namespace HubPlatform
