#include "cloud_devices/remote_interface/cloud_devices_remote_interface.h"
#include "cloud_devices/resources/cloud_devices_resources.h"
#include "cloud_devices/local_notifications/resources.h"
#include "cloud_devices/local_notifications/utils.h"
#include "service_interfaces/groups_names.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace CloudDevices {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject RemoteInterface::createCloudSubscription( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CLOUD_DEVICES,
                                      params,
                                      MessageSubClasses::HUB_CLOUD_SUBSCRIPTION_CREATE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::updateCloudSubscription( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CLOUD_DEVICES,
                                      params,
                                      MessageSubClasses::HUB_CLOUD_SUBSCRIPTION_UPDATE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::dispatchCloudSubscription( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CLOUD_DEVICES,
                                      params,
                                      MessageSubClasses::HUB_CLOUD_SUBSCRIPTION_DISPATCH,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::deleteCloudSubscription( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CLOUD_DEVICES,
                                      params,
                                      MessageSubClasses::HUB_CLOUD_SUBSCRIPTION_DELETE,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::getCloudSubscriptionsList() {
    auto abstracts = JSON_EMPTY_ARR;
    auto subscriptions =
        m_hostService.getDBClient().findAll( Resources::DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    if( subscriptions.empty() )
        return JSON( Resources::CloudRPCParams::Abstracts << abstracts );

    for( auto& subscription : subscriptions ) {
        std::string uuid = json_get_str( subscription, Resources::DB::SubscriptionFields::Id );
        json_remove( subscription, Resources::DB::SubscriptionFields::Id );
        json_add_str( subscription, Resources::RPCParams::AbstractUUID, uuid );
        abstracts.push_back( subscription );
    }
    JsonObject result;
    result[ Resources::CloudRPCParams::Abstracts ] = std::move( abstracts );
    return result;
}

std::vector< MeshbotTemplateRecord > RemoteInterface::getMeshbotTemplatesList() {
    namespace DB = LocalNotification::Resources::DB;
    namespace Fields = DB::Fields;

    std::vector< MeshbotTemplateRecord > records;

    auto info = LocalNotification::Utils::getActiveMeshbotsDataInfo( m_hostService.getDBClient() );

    if( info.empty() )
        return records;

    auto currentVersion = json_get_int( info, Fields::CurrentVersion, -1 );
    auto fileVersion = json_get_str( info, Fields::FileVersion );

    auto query = JSON( Fields::CurrentVersion << currentVersion << Fields::FileVersion << fileVersion );
    auto jsonRecords = m_hostService.getDBClient().findAll(
        LocalNotification::Resources::DB::Table::MeshbotsTemplatesDB, query, nullptr );

    records.reserve( jsonRecords.size() );
    for( auto& j : jsonRecords ) {
        records.emplace_back( MeshbotTemplateRecord::fromJson( j ) );
    }
    return records;
}

std::vector< MeshbotTemplateRecord > RemoteInterface::getMeshbotTemplatesList( int meshbotsVersion, int fileVersion ) {
    namespace DB = LocalNotification::Resources::DB;
    namespace Fields = DB::Fields;

    std::vector< MeshbotTemplateRecord > records;

    auto query = JSON( Fields::CurrentVersion << meshbotsVersion << Fields::FileVersion << fileVersion );
    auto jsonRecords = m_hostService.getDBClient().findAll(
        LocalNotification::Resources::DB::Table::MeshbotsTemplatesDB, query, nullptr );

    records.reserve( jsonRecords.size() );
    for( auto& j : jsonRecords ) {
        records.emplace_back( MeshbotTemplateRecord::fromJson( j ) );
    }
    return records;
}

LocalNotificationDataRecord RemoteInterface::getCloudDataInfo() {
    auto data = LocalNotification::Utils::getActiveCloudDataInfo( m_hostService.getDBClient() );
    return LocalNotificationDataRecord::fromJson( data );
}

LocalNotificationDataRecord RemoteInterface::getMeshbotDataInfo() {
    auto data = LocalNotification::Utils::getActiveMeshbotsDataInfo( m_hostService.getDBClient() );
    return LocalNotificationDataRecord::fromJson( data );
}

Json RemoteInterface::getLocalNotificationDataFiles() {
    auto cloudInfo = getCloudDataInfo();
    auto meshbotInfo = getMeshbotDataInfo();
    Json result = JSON( "s3_id" << cloudInfo.s3Id << "meshbots_ds3_id" << meshbotInfo.s3Id );
    return result;
}

bool RemoteInterface::switchCloudDataVersion( int cloudDataVersion, int fileVersion ) {
    return LocalNotification::Utils::setActiveCloudDataVersion(
        m_hostService.getDBClient(), cloudDataVersion, fileVersion );
}

bool RemoteInterface::switchMeshbotsVersion( int meshbotsVersion, int fileVersion ) {
    return LocalNotification::Utils::setActiveMeshbotsDataVersion(
        m_hostService.getDBClient(), meshbotsVersion, fileVersion );
}

}  // namespace CloudDevices
}  // namespace HubPlatform
