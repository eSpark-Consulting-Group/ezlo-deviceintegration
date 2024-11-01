
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"

#include "cored/tests/service/test_core_service.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Model::Resources;

class DbModifierTest : public ::testing::Test {
public:
    DbModifierTest()
    : m_dbClient( std::make_unique< HubPlatform::UT::FakeDbClient >() )
    , m_dbModifier( getDBClient(), [this]( MessageSubClasses subClass, const Json& broadcast ) {
        broadcaster( subClass, broadcast );
    } ) {}

    void SetUp() override {
        cleanDB();
        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
    }

    void TearDown() override {
        cleanDB();
        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
    }

    void broadcaster( MessageSubClasses subClass, const Json& broadcast ) {
        m_broadcastMessage.msgSubClass = subClass;
        m_broadcastMessage.data = broadcast;
    }

    IDbClient& getDBClient() {
        return *m_dbClient;
    }

    const Services::Message& getBroadcastMessage() const {
        return m_broadcastMessage;
    }

    CoreD::DeviceDbModifier& getDbModifier() {
        return m_dbModifier;
    }

    void cleanDB() {
        getDBClient().clean();
    }

    const std::string m_dbName = DBNames::DevicesDB;

    const CoreD::GatewayDbModifier::Broadcasts m_gatewayBroadcasts{ { static_cast< MessageSubClasses >( 1 ), {} },
                                                                    { static_cast< MessageSubClasses >( 2 ), {} },
                                                                    { static_cast< MessageSubClasses >( 3 ), {} } };
    const CoreD::DeviceDbModifier::Broadcasts m_deviceBroadcasts{
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_DEVICE_ADDED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_DEVICE_UPDATED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_DEVICE_REMOVED ), {} }
    };
    const CoreD::ItemDbModifier::Broadcasts m_itemBroadcasts{ { static_cast< MessageSubClasses >( 1 ), {} },
                                                              { static_cast< MessageSubClasses >( 2 ), {} },
                                                              { static_cast< MessageSubClasses >( 3 ), {} } };

private:
    std::unique_ptr< IDbClient > m_dbClient;
    CoreD::DeviceDbModifier m_dbModifier;
    Services::Message m_broadcastMessage;
};

void checkBroadcastWithTimestamp( const Json& message, const Json& expected, int codeLine ) {
    Json fwTimestamp;
    Json broadcast = message;
    EXPECT_TRUE( json_get( broadcast, System::FirmwareTimestampMs, fwTimestamp ) ) << __FILE__ << ":" << codeLine;
    json_remove( broadcast, System::FirmwareTimestampMs );
    EXPECT_EQ( broadcast, expected ) << __FILE__ << ":" << codeLine;
}

TEST_F( DbModifierTest, insert ) {
    getDbModifier().insert( JSON( "a" << 1 << DeviceFields::ReachableFieldName << true ) );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 1 );
    Json expected = JSON( "a" << 1 << DeviceFields::ReachableFieldName << true << System::SyncNotification << true );
    checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.addOperation.classType );
}

TEST_F( DbModifierTest, insertHouseModes ) {
    const std::string deviceId = "id_1";
    auto houseModes = JSON( DeviceFields::HouseModesOptions::DisarmedFieldName << JSON_ARRAY_( "1", "2" ) );
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << deviceId << "a" << 1 << DeviceFields::ProtectConfig << ""
                                                    << DeviceFields::ReachableFieldName << true
                                                    << DeviceFields::HouseModesOptionsFieldName << houseModes ) );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 1 );
    Json expected =
        JSON( db::ID_FIELD_NAME << deviceId << "a" << 1 << DeviceFields::ProtectConfig << ""
                                << DeviceFields::ReachableFieldName << true << System::SyncNotification << true );
    checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );

    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.addOperation.classType );

    auto device = getDBClient().findOne( m_dbName, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );
    EXPECT_FALSE( device.empty() );
    EXPECT_TRUE( device.find( DeviceFields::HouseModesOptionsFieldName ) != device.end() );
}

TEST_F( DbModifierTest, update ) {
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << "id_1"
                                                    << "S" << 2502 << DeviceFields::ReachableFieldName << true ) );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
    Json expected = JSON( db::ID_FIELD_NAME << "id_1"
                                            << "S" << 2502 << DeviceFields::ReachableFieldName << true
                                            << System::SyncNotification << true );
    checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );

    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.addOperation.classType );

    getDbModifier().update( "id_1", JSON( "S" << 2810 << DeviceFields::ReachableFieldName << false ) );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 1 );

    expected = JSON( "_id"
                     << "id_1" << DeviceFields::ProtectConfig << ""
                     << "S" << 2810 << DeviceFields::ReachableFieldName << false << System::SyncNotification << false );
    checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );

    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.updateOperation.classType );

    getDbModifier().update( "id_1", JSON( DeviceFields::NameFieldName << "name" ) );
    expected = JSON( "_id"
                     << "id_1" << DeviceFields::NameFieldName << "name" << DeviceFields::ProtectConfig << ""
                     << System::SyncNotification << true );
    checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );
}

TEST_F( DbModifierTest, update_initial ) {
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << "id_1" << DeviceFields::ReachableFieldName << true ) );

    getDBClient().clear( TemporaryTableNamePreffix + m_dbName );
    broadcaster( UNKNOWN, JSON_EMPTY_OBJ );

    // Different value
    {
        getDbModifier().update( "id_1", JSON( DeviceFields::ReachableFieldName << false ) );

        EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
        EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 1 );

        auto expected = JSON( "_id"
                              << "id_1" << DeviceFields::ReachableFieldName << false << DeviceFields::ProtectConfig
                              << "" << System::SyncNotification << false );
        checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.updateOperation.classType );
    }

    getDBClient().clear( TemporaryTableNamePreffix + m_dbName );
    broadcaster( UNKNOWN, JSON_EMPTY_OBJ );

    // Same value
    {
        getDbModifier().update( "id_1", JSON( DeviceFields::ReachableFieldName << true ) );

        EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
        EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 0 );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, UNKNOWN );
        EXPECT_EQ( getBroadcastMessage().data, JSON_EMPTY_OBJ );
    }
}

TEST_F( DbModifierTest, updateFloat ) {
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << "id_1"
                                                    << "value" << 25.5f << DeviceFields::ReachableFieldName << true ) );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.addOperation.classType );

    {
        getDbModifier().update( "id_1", JSON( "value" << 25.55f ) );

        auto expected =
            JSON( "_id"
                  << "id_1"
                  << "value" << 25.55f << DeviceFields::ProtectConfig << "" << System::SyncNotification << false );
        checkBroadcastWithTimestamp( getBroadcastMessage().data, expected, __LINE__ );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.updateOperation.classType );

        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );

        getDbModifier().update( "id_1", JSON( "value" << 25.550005f ) );
        EXPECT_EQ( getBroadcastMessage().msgSubClass, UNKNOWN );
        EXPECT_EQ( getBroadcastMessage().data, JSON_EMPTY_OBJ );

        const auto result = getDBClient().findOne( m_dbName, JSON( db::ID_FIELD_NAME << "id_1" ), nullptr );
        EXPECT_EQ( result,
                   JSON( "_id"
                         << "id_1" << DeviceFields::ReachableFieldName << true << "value" << 25.55f ) );
    }
    {
        getDbModifier().update( "id_1", JSON( "value" << -25.55f ) );

        checkBroadcastWithTimestamp( getBroadcastMessage().data,
                                     JSON( "_id"
                                           << "id_1"
                                           << "value" << -25.55f << DeviceFields::ProtectConfig << ""
                                           << System::SyncNotification << false ),
                                     __LINE__ );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.updateOperation.classType );

        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );

        getDbModifier().update( "id_1", JSON( "value" << -25.550005f ) );
        EXPECT_EQ( getBroadcastMessage().msgSubClass, UNKNOWN );
        EXPECT_EQ( getBroadcastMessage().data, JSON_EMPTY_OBJ );

        const auto result = getDBClient().findOne( m_dbName, JSON( db::ID_FIELD_NAME << "id_1" ), nullptr );
        EXPECT_EQ( result,
                   JSON( "_id"
                         << "id_1" << DeviceFields::ReachableFieldName << true << "value" << -25.55f ) );
    }
}

TEST_F( DbModifierTest, complexUpdate ) {
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << "id"
                                                    << "S" << 2502 << "obj"
                                                    << JSON( "key"
                                                             << "value" ) ) );

    const auto updatePortion = JSON( "S" << 2810 << "obj"
                                         << JSON( "key"
                                                  << "S" ) );
    getDbModifier().update( "id", updatePortion );

    const auto result = getDBClient().findOne( m_dbName, JSON_EMPTY_OBJ, nullptr );

    EXPECT_EQ( result,
               JSON( db::ID_FIELD_NAME << "id"
                                       << "S" << 2810
                                       << JSON( "obj" << JSON( "key"
                                                               << "S" ) ) ) );

    checkBroadcastWithTimestamp(
        getBroadcastMessage().data,
        JSON( db::ID_FIELD_NAME << "id"
                                << "S" << 2810
                                << JSON( "obj" << JSON( "key"
                                                        << "S" ) )
                                << DeviceFields::ProtectConfig << "" << System::SyncNotification << false ),
        __LINE__ );

    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.updateOperation.classType );

    getDbModifier().update( "id", JSON( DeviceFields::NameFieldName << "name" ) );
    checkBroadcastWithTimestamp(
        getBroadcastMessage().data,
        JSON( db::ID_FIELD_NAME << "id" << DeviceFields::NameFieldName << "name" << DeviceFields::ProtectConfig << ""
                                << System::SyncNotification << true ),
        __LINE__ );
}

TEST_F( DbModifierTest, updateHouseModes ) {
    std::string deviceId = "id_1";
    const auto initialObject =
        JSON( db::ID_FIELD_NAME << deviceId << "S" << 2502 << DeviceFields::ReachableFieldName << true );

    auto houseModes = JSON( DeviceFields::HouseModesOptions::DisarmedFieldName << JSON_ARRAY_( "1", "2" ) );
    JsonBuilder builder;
    Json insertObject = initialObject;

    {
        insertObject[ DeviceFields::HouseModesOptionsFieldName ] = houseModes;
        getDbModifier().insert( insertObject );

        EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );

        builder.getJson() = initialObject;
        checkBroadcastWithTimestamp(
            getBroadcastMessage().data, builder << System::SyncNotification << true, __LINE__ );
        EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.addOperation.classType );

        auto device = getDBClient().findOne( m_dbName, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );
        EXPECT_FALSE( device.empty() );
        EXPECT_TRUE( device.find( DeviceFields::HouseModesOptionsFieldName ) != device.end() );
    }

    {
        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
        Json modifiedObject = insertObject;
        modifiedObject[ DeviceFields::HouseModesOptionsFieldName ]
                      [ DeviceFields::HouseModesOptions::AlarmsOffFieldName ] = JSON_ARRAY_( "3", "4" );

        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
        getDbModifier().update( deviceId, modifiedObject );

        EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );
        EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 1 );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, UNKNOWN )
            << "Should not be broadcast when only houseModesOptions were changed";
        EXPECT_TRUE( getBroadcastMessage().data.empty() )
            << "Should not be broadcast when only houseModesOptions were changed: "
            << cc::j( getBroadcastMessage().data );

        auto device = getDBClient().findOne( m_dbName, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );
        EXPECT_FALSE( device.empty() );
        EXPECT_TRUE( device.find( DeviceFields::HouseModesOptionsFieldName ) != device.end() );
    }

    const char* modifiedName = "modified device name";
    {
        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
        Json modifiedObject = insertObject;
        modifiedObject[ DeviceFields::HouseModesOptionsFieldName ] = houseModes;
        modifiedObject[ DeviceFields::NameFieldName ] = modifiedName;

        getDbModifier().update( deviceId, modifiedObject );
        auto expectedBroadcast =
            JSON( db::ID_FIELD_NAME << deviceId << DeviceFields::NameFieldName << modifiedName
                                    << DeviceFields::ProtectConfig << "" << System::SyncNotification << true );
        // "Should not contain internal fields"
        checkBroadcastWithTimestamp( getBroadcastMessage().data, expectedBroadcast, __LINE__ );
    }

    {
        broadcaster( UNKNOWN, JSON_EMPTY_OBJ );
        getDbModifier().remove( deviceId );

        auto expectedBroadcast = JSON( db::ID_FIELD_NAME << deviceId << DeviceFields::ProtectConfig << ""
                                                         << System::SyncNotification << true );
        // Should not contain internal fields
        checkBroadcastWithTimestamp( getBroadcastMessage().data, expectedBroadcast, __LINE__ );
    }
}

TEST_F( DbModifierTest, remove ) {
    getDbModifier().insert( JSON( db::ID_FIELD_NAME << "id_1" ) );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 1 );

    getDbModifier().remove( "id_1" );

    EXPECT_EQ( getDBClient().count( m_dbName, JSON_EMPTY_OBJ ), 0 );
    EXPECT_EQ( getDBClient().count( ( TemporaryTableNamePreffix + m_dbName ), JSON_EMPTY_OBJ ), 0 );

    checkBroadcastWithTimestamp(
        getBroadcastMessage().data,
        JSON( db::ID_FIELD_NAME << "id_1" << DeviceFields::ProtectConfig << "" << System::SyncNotification << true ),
        __LINE__ );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_deviceBroadcasts.removeOperation.classType );
}

}  // namespace UT
}  // namespace HubPlatform
