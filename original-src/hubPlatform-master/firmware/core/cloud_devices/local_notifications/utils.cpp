#include "utils.h"
#include "resources.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace CloudDevices {
namespace LocalNotification {
namespace Utils {

void addCloudDataInfo( IDbClient& dbClient,
                       const int version,
                       const int fileVersion,
                       const std::string& url,
                       const std::string& s3Id ) {
    using namespace LocalNotification::Resources::DB;

    auto currentTimestamp = std::chrono::system_clock::now();
    auto timeSeconds = std::chrono::system_clock::to_time_t( currentTimestamp );

    auto record =
        JSON( Fields::RecordType << Fields::CloudDataInfo << Fields::CurrentVersion << version << Fields::FileVersion
                                 << fileVersion << "url" << url << "s3_id" << s3Id << "modified" << timeSeconds );

    auto query = JSON( Fields::RecordType << Fields::CloudDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );

    dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

void addMeshbotsDataInfo( IDbClient& dbClient,
                          const int version,
                          const int fileVersion,
                          const std::string& url,
                          const std::string& s3Id ) {
    using namespace LocalNotification::Resources::DB;

    auto currentTimestamp = std::chrono::system_clock::now();
    auto timeSeconds = std::chrono::system_clock::to_time_t( currentTimestamp );

    auto record =
        JSON( Fields::RecordType << Fields::MeshbotsDataInfo << Fields::CurrentVersion << version << Fields::FileVersion
                                 << fileVersion << "url" << url << "s3_id" << s3Id << "modified" << timeSeconds );

    auto query = JSON( Fields::RecordType << Fields::MeshbotsDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );

    dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

bool setActiveCloudDataVersion( IDbClient& dbClient, const int version, const int fileVersion ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( Fields::RecordType << Fields::CloudDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );

    auto data = dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
    if( data.empty() ) {
        LOGGER_ERROR( cc::error( "Unable to set active cloud data version, record not found. " ) +
                      cc::note( "version: " ) + cc::num10( version ) + cc::note( "file_version: " ) +
                      cc::num10( fileVersion ) );
        return false;
    }

    data[ db::ID_FIELD_NAME ] = Fields::ActiveCloudDataInfo;
    data[ Fields::RecordType ] = Fields::ActiveCloudDataInfo;

    dbClient.update( Table::LocalNotificationsDB, nullptr, data, true, false );

    return true;
}

bool setActiveMeshbotsDataVersion( IDbClient& dbClient, const int version, const int fileVersion ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( Fields::RecordType << Fields::MeshbotsDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );

    auto data = dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
    if( data.empty() ) {
        LOGGER_ERROR( cc::error( "Unable to set active meshbot data version, record not found. " ) +
                      cc::note( "version: " ) + cc::num10( version ) + cc::note( "file_version: " ) +
                      cc::num10( fileVersion ) );
        return false;
    }

    data[ db::ID_FIELD_NAME ] = Fields::ActiveMeshbotsDataInfo;
    data[ Fields::RecordType ] = Fields::ActiveMeshbotsDataInfo;

    dbClient.update( Table::LocalNotificationsDB, nullptr, data, true, false );

    return true;
}

Json getCloudDataInfo( IDbClient& dbClient, const int version, const int fileVersion ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( Fields::RecordType << Fields::CloudDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );
    return dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
}

Json getMeshbotsDataInfo( IDbClient& dbClient, const int version, const int fileVersion ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( Fields::RecordType << Fields::MeshbotsDataInfo << Fields::CurrentVersion << version
                                          << Fields::FileVersion << fileVersion );
    return dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
}

Json getActiveCloudDataInfo( IDbClient& dbClient ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( db::ID_FIELD_NAME << Fields::ActiveCloudDataInfo );
    return dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
}

Json getActiveMeshbotsDataInfo( IDbClient& dbClient ) {
    using namespace LocalNotification::Resources::DB;
    auto query = JSON( db::ID_FIELD_NAME << Fields::ActiveMeshbotsDataInfo );
    return dbClient.findOne( Table::LocalNotificationsDB, query, nullptr );
}

void removeUnusedCloudDataVersions( IDbClient& dbClient ) {
    namespace Fields = Resources::DB::Fields;

    const auto tableName = Resources::DB::Table::LocalNotificationsDB;

    const std::vector< std::string > types = { Fields::LocationData,          Fields::UserData,     Fields::UserData,
                                               Fields::PartnerData,           Fields::SettingData,  Fields::ProtectData,
                                               Fields::RegisteredDevicesData, Fields::CloudDataInfo };

    auto activeInfo = getActiveCloudDataInfo( dbClient );
    if( activeInfo.empty() )
        return;

    int activeVersion = json_get_int( activeInfo, Fields::CurrentVersion );
    int activeFileVersion = json_get_int( activeInfo, Fields::FileVersion );

    for( auto& type : types ) {
        auto query = JSON( Fields::RecordType << type );

        auto records = dbClient.findAll( tableName, query, nullptr );
        for( auto& data : records ) {
            auto dataVersion = json_get_int( data, Fields::CurrentVersion );
            auto fileVersion = json_get_int( data, Fields::FileVersion );

            if( dataVersion == activeVersion && fileVersion == activeFileVersion )
                continue;
            auto id = json_get_str( data, db::ID_FIELD_NAME );
            dbClient.remove( tableName, JSON( db::ID_FIELD_NAME << id ) );
        }
    }
}

void removeUnusedMeshbotsVersions( IDbClient& dbClient ) {
    namespace Fields = Resources::DB::Fields;
    const auto tableName = Resources::DB::Table::MeshbotsTemplatesDB;

    auto activeInfo = getActiveMeshbotsDataInfo( dbClient );
    if( activeInfo.empty() )
        return;

    int activeVersion = json_get_int( activeInfo, Fields::CurrentVersion );
    int activeFileVersion = json_get_int( activeInfo, Fields::FileVersion );

    auto genericData = dbClient.findAll( tableName, JSON_EMPTY_OBJ, nullptr );
    for( auto& data : genericData ) {
        auto dataVersion = json_get_int( data, Fields::CurrentVersion );
        auto fileVersion = json_get_int( data, Fields::FileVersion );

        if( dataVersion == activeVersion && fileVersion == activeFileVersion )
            continue;

        auto id = json_get_str( data, db::ID_FIELD_NAME );
        dbClient.remove( tableName, JSON( db::ID_FIELD_NAME << id ) );
    }

    {
        const auto tableName = Resources::DB::Table::LocalNotificationsDB;
        auto query = JSON( Fields::RecordType << Fields::MeshbotsDataInfo );
        auto records = dbClient.findAll( tableName, query, nullptr );
        for( auto& data : records ) {
            auto dataVersion = json_get_int( data, Fields::CurrentVersion );
            auto fileVersion = json_get_int( data, Fields::FileVersion );

            if( dataVersion == activeVersion && fileVersion == activeFileVersion )
                continue;
            auto id = json_get_str( data, db::ID_FIELD_NAME );
            dbClient.remove( tableName, JSON( db::ID_FIELD_NAME << id ) );
        }
    }
}

}  // namespace Utils
}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
