
#include "test_utils/test_utils.h"

#include "2gig/2gig_devices_repository.h"
#include "2gig/tests/fakes/in_memory_persistence_client.h"
#include "2gig/resources/resources.h"

#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Migration a few old devices
DONE    2. Migration of empty storage
DONE    3. Migration partially broken storage file
DONE    4. Migration already added devices

*/

namespace HubPlatform {
namespace TwoGig {
namespace UT {

using namespace ::testing;

struct DevicesMigrationTestSuite : public testing::Test {
    DevicesMigrationTestSuite()
    : m_devices( m_database ) {}

    bool isDeviceExistInDB( uint32_t serial ) {
        return m_database.has( Resources::DB::TableNames::Devices,
                               JSON( db::ID_FIELD_NAME << std::to_string( serial ) ) );
    }

protected:
    ::HubPlatform::UT::FakeDbClient m_database;
    DevicesRepositoryImpl m_devices;
};

TEST_F( DevicesMigrationTestSuite, migrationAFewOldDevices ) {
    InMemoryPersistenceClient persistenceClient;

    persistenceClient.setData( {
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*count*/
        0x01, 0x00, 0x00, 0x00,                         /*item*/
        0x02, 0x00, 0x00, 0x00,                         /*item*/
        0x03, 0x00, 0x00, 0x00,                         /*item*/
    } );

    ASSERT_TRUE( m_devices.migrateOldDevices( persistenceClient ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_TRUE( isDeviceExistInDB( 2 ) );
    ASSERT_TRUE( isDeviceExistInDB( 3 ) );
    ASSERT_FALSE( isDeviceExistInDB( 4 ) );
}

TEST_F( DevicesMigrationTestSuite, migrationEmptyStorage ) {
    InMemoryPersistenceClient persistenceClient;

    persistenceClient.setData( {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*count*/
    } );

    ASSERT_TRUE( m_devices.migrateOldDevices( persistenceClient ) );

    ASSERT_FALSE( isDeviceExistInDB( 1 ) );
    ASSERT_FALSE( isDeviceExistInDB( 2 ) );
    ASSERT_FALSE( isDeviceExistInDB( 3 ) );
    ASSERT_FALSE( isDeviceExistInDB( 4 ) );
}

TEST_F( DevicesMigrationTestSuite, migrationPartiallyBrokenStorageFile ) {
    InMemoryPersistenceClient persistenceClient;

    persistenceClient.setData( {
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, /*count*/
        0x01,
        0x00,
        0x00,
        0x00, /*item*/
        0x02,
        0x00,
        0x00,
        0x00, /*item*/
    } );

    ASSERT_FALSE( m_devices.migrateOldDevices( persistenceClient ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_TRUE( isDeviceExistInDB( 2 ) );
    ASSERT_FALSE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesMigrationTestSuite, migrationAlreadyExistDevices ) {
    InMemoryPersistenceClient persistenceClient;

    ASSERT_TRUE( m_devices.addDevice( { 2 } ) );

    persistenceClient.setData( {
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*count*/
        0x01, 0x00, 0x00, 0x00,                         /*item*/
        0x02, 0x00, 0x00, 0x00,                         /*item*/
        0x03, 0x00, 0x00, 0x00,                         /*item*/
    } );

    ASSERT_TRUE( m_devices.migrateOldDevices( persistenceClient ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_TRUE( isDeviceExistInDB( 2 ) );
    ASSERT_TRUE( isDeviceExistInDB( 3 ) );
    ASSERT_FALSE( isDeviceExistInDB( 4 ) );
}

}  // namespace UT
}  // namespace TwoGig
}  // namespace HubPlatform
