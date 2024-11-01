#include <gtest/gtest.h>
#include "json_utils/json_utils.h"
#include "cloud_devices/local_notifications/data_segments_deserializer.h"
#include "cloud_devices/tests/fakes/fake_cloud_devices_service.h"
#include "cloud_devices/local_notifications/resources.h"
#include "provision_data_utils/provision_data_manager.h"

namespace HubPlatform {
namespace UT {

using namespace CloudDevices;

class DataSegmentsDeserializerTest : public FakeCloudDevicesService {
protected:
    static void SetUpTestCase();
};

void DataSegmentsDeserializerTest::SetUpTestCase() {
    FakeCloudDevicesService::SetUpTestCase();
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

TEST_F( DataSegmentsDeserializerTest, locationDeseriliazer ) {
    auto data = JSON( "oem_id" << 5 << "location_uuid"
                               << "123456"
                               << "account_id" << 3253252 );

    LocalNotification::LocationDeserializer locationDeserializer( getDBClient() );
    locationDeserializer.fromJson( data );
    auto answer = locationDeserializer.toJson();
    data[ "permissions" ] = JSON_ARRAY_();
    EXPECT_EQ( data, answer );
}

TEST_F( DataSegmentsDeserializerTest, userDeseriliazer ) {
    auto data = JSON( "user_uuid"
                      << "123456"
                      << "user_id" << 3253252 << "permission_role" << 35 );

    LocalNotification::UserDeserializer userDeserializer( getDBClient() );
    userDeserializer.fromJson( data );
    data[ "email_validated" ] = 0;
    data[ "notification_categories" ] = JSON_ARRAY_();
    data[ "permissions" ] = JSON_ARRAY_();

    auto answer = userDeserializer.toJson();
    // removing encrypted fields for the comparison
    answer.erase( "email" );
    answer.erase( "arn" );
    answer.erase( "first_name" );
    answer.erase( "last_name" );
    answer.erase( "username" );

    EXPECT_EQ( data, answer );
}

TEST_F( DataSegmentsDeserializerTest, userDeseriliazerEncryptedData ) {
    auto data = JSON( "user_uuid"
                      << "123456"
                      << "user_id" << 3253252 << "permission_role" << 35 << "email"
                      << "test@ezlo.com"
                      << "username"
                      << "test_user" );

    LocalNotification::UserDeserializer userDeserializer( getDBClient() );
    userDeserializer.fromJson( data );
    auto answer = userDeserializer.toJson();

    EXPECT_NE( data[ "email" ], answer[ "email" ] );        // due to encryption
    EXPECT_NE( data[ "username" ], answer[ "username" ] );  // due to encryption
}

TEST_F( DataSegmentsDeserializerTest, partnerDeseriliazer ) {
    auto data = JSON_EMPTY_OBJ;
    data[ "partner" ] = JSON( "partner_uuid"
                              << "123456" );

    data[ "custom" ] = JSON( "custom_field"
                             << "custom_value" );

    LocalNotification::PartnerDeserializer partnerDeserializer( getDBClient() );
    partnerDeserializer.fromJson( data );
    auto answer = partnerDeserializer.toJson();
    EXPECT_EQ( data, answer );
}

TEST_F( DataSegmentsDeserializerTest, registeredDevicesDeseriliazer ) {
    auto data =
        JSON( "uuid"
              << "123456"
              << "name"
              << "myDevice"
              << "capabilities" << JSON_ARRAY_( "power", "power_status" ) << "outbound_connectivity" << JSON_ARRAY_() );

    LocalNotification::RegisteredDevicesDeserializer registeredDevicesDeserializer( getDBClient() );
    registeredDevicesDeserializer.fromJson( data );
    auto answer = registeredDevicesDeserializer.toJson();
    EXPECT_EQ( data, answer );
}

TEST_F( DataSegmentsDeserializerTest, locationDbData ) {
    using namespace LocalNotification::Resources::DB;
    auto data = JSON( "oem_id" << 5 << "location_uuid"
                               << "123456"
                               << "account_id" << 3253252 );

    LocalNotification::LocationDeserializer locationDeserializer( getDBClient() );
    locationDeserializer.fromJson( data );
    locationDeserializer.addRecordToDb( 1, 1 );
    auto records = getDBClient().findAll(
        Table::LocalNotificationsDB, JSON( Fields::RecordType << Fields::LocationData ), nullptr );

    ASSERT_TRUE( records.size() == 1 );
    EXPECT_EQ( json_get_int( records[ 0 ], "oem_id" ), data[ "oem_id" ] );
    EXPECT_EQ( json_get_str( records[ 0 ], "location_uuid" ), data[ "location_uuid" ] );
    EXPECT_EQ( json_get_int( records[ 0 ], "account_id" ), data[ "account_id" ] );
    EXPECT_EQ( json_get_int( records[ 0 ], Fields::CurrentVersion ), 1 );

    auto newData = JSON( "oem_id" << 6 << "location_uuid"
                                  << "123456"
                                  << "account_id" << 11111 );
    locationDeserializer.fromJson( newData );
    locationDeserializer.addRecordToDb( 2, 2 );

    records = getDBClient().findAll(
        Table::LocalNotificationsDB, JSON( Fields::RecordType << Fields::LocationData ), nullptr );
    ASSERT_EQ( records.size(), 2 );
    EXPECT_EQ( json_get_int( records[ 1 ], "oem_id" ), 6 );
    EXPECT_EQ( json_get_str( records[ 1 ], "location_uuid" ), "123456" );
    EXPECT_EQ( json_get_int( records[ 1 ], "account_id" ), 11111 );
    EXPECT_EQ( json_get_int( records[ 1 ], Fields::CurrentVersion ), 2 );
}

TEST_F( DataSegmentsDeserializerTest, userDeseriliazerDecryptData ) {
    using namespace LocalNotification::Resources::DB;
    auto data = JSON( "user_uuid"
                      << "123456"
                      << "user_id" << 3253252 << "permission_role" << 35 << "email"
                      << "test@ezlo.com"
                      << "username"
                      << "test_user" );

    LocalNotification::UserDeserializer userDeserializer( getDBClient() );
    userDeserializer.fromJson( data );
    userDeserializer.addRecordToDb( 1, 1 );

    auto records =
        getDBClient().findAll( Table::LocalNotificationsDB, JSON( Fields::RecordType << Fields::UserData ), nullptr );

    ASSERT_TRUE( records.size() == 1 );
    EXPECT_EQ( json_get_int( records[ 0 ], "user_id" ), data[ "user_id" ] );
    EXPECT_EQ( json_get_str( records[ 0 ], "user_uuid" ), data[ "user_uuid" ] );
    EXPECT_EQ( json_get_int( records[ 0 ], "permission_role" ), data[ "permission_role" ] );
    EXPECT_EQ( json_get_int( records[ 0 ], Fields::CurrentVersion ), 1 );

    auto& crypto = ProvisionDataUtils::getManager().getProvisionData();
    auto email = json_get_str( records[ 0 ], "email" );
    std::string decryptedEmail = crypto.decryptSensitiveData( email );

    EXPECT_EQ( decryptedEmail, data[ "email" ] );
}

TEST_F( DataSegmentsDeserializerTest, meshbotsTemplatesData ) {
    using namespace LocalNotification::Resources::DB;
    auto uuid1 = "uuid1";
    auto uuid2 = "uuid2";
    auto uuid3 = "uuid3";
    auto data = JSON( "uuid" << uuid1 << "title"
                             << "testing"
                             << "description"
                             << "meshbot uuid data testing"
                             << "id"
                             << "id_test"
                             << "partner_uuid"
                             << "4564"
                             << "oem_id"
                             << "2222"
                             << "local_meshbot" << JSON_EMPTY_OBJ );

    LocalNotification::MeshbotUuidDeserializer meshbotUuidDeserializer( getDBClient() );
    meshbotUuidDeserializer.fromJson( data );
    meshbotUuidDeserializer.addRecordToDb( 1, 1 );

    data[ "uuid" ] = uuid2;
    meshbotUuidDeserializer.fromJson( data );
    meshbotUuidDeserializer.addRecordToDb( 1, 1 );

    data[ "uuid" ] = uuid3;
    meshbotUuidDeserializer.fromJson( data );
    meshbotUuidDeserializer.addRecordToDb( 1, 1 );
    auto records = getDBClient().findAll( Table::MeshbotsTemplatesDB, JSON_EMPTY_OBJ, nullptr );

    ASSERT_TRUE( records.size() == 3 );
    EXPECT_EQ( json_get_str( records[ 0 ], "uuid" ), uuid1 );
    EXPECT_EQ( json_get_str( records[ 1 ], "uuid" ), uuid2 );
    EXPECT_EQ( json_get_str( records[ 2 ], "uuid" ), uuid3 );
}

}  // namespace UT
}  // namespace HubPlatform
