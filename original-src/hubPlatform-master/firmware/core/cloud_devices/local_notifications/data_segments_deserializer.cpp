#include "data_segments_deserializer.h"
#include "json_utils/json_utils.h"
#include "resources.h"
#include "provision_data_utils/provision_data_manager.h"

namespace HubPlatform {

namespace CloudDevices {
namespace LocalNotification {

void LocationDeserializer::fromJson( const JsonObject& data ) {
    using namespace Resources::DataSegments;
    m_oemId = json_get_int( data, LocationDataSegments::OemId );
    m_locationUuid = json_get_str( data, LocationDataSegments::LocationId );
    m_accountId = json_get_int( data, LocationDataSegments::AccountId );
    json_get_arr( data, LocationDataSegments::Permissions, m_permissions );
}

JsonObject LocationDeserializer::toJson() {
    using namespace Resources::DataSegments;
    JsonObject data;
    json_add_int( data, LocationDataSegments::OemId, m_oemId );
    json_add_str( data, LocationDataSegments::LocationId, m_locationUuid );
    json_add_int( data, LocationDataSegments::AccountId, m_accountId );
    data[ LocationDataSegments::Permissions ] = m_permissions;
    return data;
}

LocationDeserializer::LocationDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void LocationDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::LocationData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::LocationData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

void UserDeserializer::fromJson( const JsonObject& data ) {
    using namespace Resources::DataSegments;
    m_userUuid = json_get_str( data, UserDataSegments::UserUuid );
    m_userId = json_get_int( data, UserDataSegments::UserId );
    m_permissionRole = json_get_int( data, UserDataSegments::PermissionRole );
    m_email = encryptData( json_get_str( data, UserDataSegments::Email ) );
    m_emailValidated = json_get_int( data, UserDataSegments::EmailValidated, 0 );
    m_arn = encryptData( json_get_str( data, UserDataSegments::Arn ) );
    m_firstName = encryptData( json_get_str( data, UserDataSegments::FirstName ) );
    m_lastName = encryptData( json_get_str( data, UserDataSegments::LastName ) );
    m_userName = encryptData( json_get_str( data, UserDataSegments::Username ) );
    json_get_arr( data, UserDataSegments::NotificationCategories, m_notificationCategories );
    json_get_arr( data, UserDataSegments::Permissions, m_permissions );
}

JsonObject UserDeserializer::toJson() {
    using namespace Resources::DataSegments;
    JsonObject data;
    json_add_int( data, UserDataSegments::UserId, m_userId );
    json_add_str( data, UserDataSegments::UserUuid, m_userUuid );
    json_add_int( data, UserDataSegments::PermissionRole, m_permissionRole );
    json_add_str( data, UserDataSegments::Email, m_email );
    json_add_int( data, UserDataSegments::EmailValidated, m_emailValidated );
    json_add_str( data, UserDataSegments::Arn, m_arn );
    json_add_str( data, UserDataSegments::FirstName, m_firstName );
    json_add_str( data, UserDataSegments::LastName, m_lastName );
    json_add_str( data, UserDataSegments::Username, m_userName );
    data[ UserDataSegments::NotificationCategories ] = m_notificationCategories;
    data[ UserDataSegments::Permissions ] = m_permissions;
    return data;
}

std::string UserDeserializer::encryptData( const std::string& data ) {
    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    return crypto.encryptSensitiveData( data );
}

std::string UserDeserializer::decryptData( const std::string& data ) {
    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    return crypto.decryptSensitiveData( data );
}

UserDeserializer::UserDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void UserDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::UserData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::UserData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

void PartnerDeserializer::fromJson( const JsonObject& data ) {
    m_data = data;
}

JsonObject PartnerDeserializer::toJson() {
    return m_data;
}

PartnerDeserializer::PartnerDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void PartnerDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::PartnerData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::PartnerData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

void SettingsDeserializer::fromJson( const JsonObject& data ) {
    m_data = data;
}

JsonObject SettingsDeserializer::toJson() {
    return m_data;
}

SettingsDeserializer::SettingsDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void SettingsDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::SettingData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::SettingData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

JsonObject ProtectDeserializer::toJson() {
    using namespace Resources::DataSegments;
    JsonObject data;
    json_add_str( data, ProtectDataSegments::Provider, m_provider );
    return data;
}

void ProtectDeserializer::fromJson( const JsonObject& data ) {
    using namespace Resources::DataSegments;
    m_provider = json_get_str( data, ProtectDataSegments::Provider );
}

ProtectDeserializer::ProtectDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void ProtectDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::ProtectData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::ProtectData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

void RegisteredDevicesDeserializer::fromJson( const JsonObject& data ) {
    using namespace Resources::DataSegments;
    m_uuid = json_get_str( data, RegisteredDevicesDataSegments::Uuid );
    m_name = json_get_str( data, RegisteredDevicesDataSegments::Name );
    json_get_arr( data, RegisteredDevicesDataSegments::Capabilities, m_capabilities );
    json_get_arr( data, RegisteredDevicesDataSegments::OutBoundConnectivity, m_outboundConnectivity );
}

JsonObject RegisteredDevicesDeserializer::toJson() {
    using namespace Resources::DataSegments;
    JsonObject data;
    json_add_str( data, RegisteredDevicesDataSegments::Uuid, m_uuid );
    json_add_str( data, RegisteredDevicesDataSegments::Name, m_name );
    data[ RegisteredDevicesDataSegments::Capabilities ] = m_capabilities;
    data[ RegisteredDevicesDataSegments::OutBoundConnectivity ] = m_outboundConnectivity;
    return data;
}

RegisteredDevicesDeserializer::RegisteredDevicesDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void RegisteredDevicesDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    auto record = toJson();
    record[ Fields::RecordType ] = Fields::RegisteredDevicesData;
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::RecordType ] = Fields::RegisteredDevicesData;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::LocalNotificationsDB, query, record, true, false );
}

DataSegmentsDeserializer::DataSegmentsDeserializer( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

MeshbotUuidDeserializer::MeshbotUuidDeserializer( IDbClient& dbClient )
: DataSegmentsDeserializer( dbClient ) {}

void MeshbotUuidDeserializer::fromJson( const JsonObject& data ) {
    using namespace Resources::DataSegments;
    m_record.uuid = json_get_str( data, MeshbotUuidDataSegments::Uuid );
    m_record.title = json_get_str( data, MeshbotUuidDataSegments::Title );
    m_record.description = json_get_str( data, MeshbotUuidDataSegments::Description );
    m_record.id = json_get_str( data, MeshbotUuidDataSegments::Id );
    m_record.partnerUUID = json_get_str( data, MeshbotUuidDataSegments::PartnerUuid );
    m_record.oemId = json_get_str( data, MeshbotUuidDataSegments::OemId );
    m_record.localMeshbot = json_get_str( data, MeshbotUuidDataSegments::LocalMeshbot );
    json_get_arr( data, MeshbotUuidDataSegments::Rules, m_record.rules );
}

JsonObject MeshbotUuidDeserializer::toJson() {
    return m_record.toJson();
}

void MeshbotUuidDeserializer::addRecordToDb( const int currentVersion, const int fileVersion ) {
    using namespace Resources::DB;
    m_record.version = std::to_string( fileVersion ) + "_" + std::to_string( currentVersion );
    auto record = toJson();
    record[ Fields::CurrentVersion ] = currentVersion;
    record[ Fields::FileVersion ] = fileVersion;

    Json query;
    query[ Fields::CurrentVersion ] = currentVersion;
    query[ Fields::FileVersion ] = fileVersion;

    m_dbClient.update( Table::MeshbotsTemplatesDB, query, record, true, false );
}

std::string MeshbotUuidDeserializer::getUuid() {
    return m_record.uuid;
}

}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
