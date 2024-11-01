#include "connection_report_db_info.h"
#include "resources.h"

namespace HubPlatform {
namespace Uid {
namespace Reporter {

using namespace HubPlatform::Uid::Reporter::Resources;

ConnectionReportDbInfo::ConnectionReportDbInfo( const Json& record ) {
    fromJson( record );
}

std::string ConnectionReportDbInfo::getId() const {
    return m_id;
}

std::chrono::seconds ConnectionReportDbInfo::getUptime() const {
    return m_uptime;
}

int ConnectionReportDbInfo::getReconnects() const {
    return m_reconnects;
}

void ConnectionReportDbInfo::setId( const std::string& id ) {
    m_id = id;
}

void ConnectionReportDbInfo::setUptime( std::chrono::seconds timestamp ) {
    m_uptime = timestamp;
}

void ConnectionReportDbInfo::setReconnects( int reconnects ) {
    m_reconnects = reconnects;
}

Json ConnectionReportDbInfo::toJson() const {
    using namespace HubPlatform::Uid::Reporter::Resources;
    JsonObject infoItem;
    infoItem[ DbInfo::Id ] = m_id;
    infoItem[ DbInfo::NmaUptime ] = m_uptime.count();
    infoItem[ DbInfo::NmaReconnects ] = m_reconnects;
    return infoItem;
}

bool ConnectionReportDbInfo::fromJson( const JsonObject& data ) {
    using namespace HubPlatform::Uid::Reporter::Resources;
    JsonObject infoItem;
    try {
        auto id = data.find( DbInfo::Id );
        if( id != data.end() && id->is_string() ) {
            m_id = json_get_str( data, DbInfo::Id );
        }

        auto uptime = data.find( DbInfo::NmaUptime );
        if( uptime != data.end() && uptime->is_number() ) {
            m_uptime = std::chrono::seconds( json_get_int( data, DbInfo::NmaUptime ) );
        }

        auto reconnects = data.find( DbInfo::NmaReconnects );
        if( reconnects != data.end() && reconnects->is_number() ) {
            m_reconnects = json_get_int( data, DbInfo::NmaReconnects );
        }

        return true;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to extract connection report: " ) + cc::note( ex.what() ) );
    }

    return false;
}

void ConnectionReportDbInfo::createDbRecord( JsonObject query, const std::string dbName, IDbClient& dbClient ) {
    if( query.empty() ) {
        dbClient.insert( dbName, JSON_EMPTY_OBJ, true );
    }
    dbClient.insert( dbName, query, true );
}

void ConnectionReportDbInfo::updateDbRecord( JsonObject updateObj, const std::string& dbName, IDbClient& dbClient ) {
    dbClient.update( dbName, JSON_EMPTY_OBJ, updateObj, false, true );
}

Json ConnectionReportDbInfo::loadFromDb( const std::string& dbName, IDbClient& dbClient ) {
    Json record = JSON_EMPTY_OBJ;
    record = dbClient.findOne( dbName, JSON_EMPTY_OBJ, nullptr );
    return record;
}

void ConnectionReportDbInfo::deleteDbRecord( const std::string& id, const std::string& dbName, IDbClient& dbClient ) {
    dbClient.remove( dbName, JSON( HubPlatform::Uid::Reporter::Resources::DbInfo::Id << id ) );
}

}  // namespace Reporter
}  // namespace Uid
}  // namespace HubPlatform
