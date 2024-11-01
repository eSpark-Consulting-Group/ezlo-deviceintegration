
#include "test_utils/test_utils.h"

#include "2gig/2gig_devices_repository.h"
#include "2gig/tests/fakes/in_memory_persistence_client.h"
#include "2gig/resources/resources.h"

#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Add device
DONE    2. Has device checking
DONE    3. Removing devices
DONE    4. Removing all devices
DONE    5. Add existing device
DONE    6. Remove existing device
DONE    7. Add device with multiple TXIDs
DONE    7. Remove device with multiple TXIDs
DONE    8. For each device
DONE    9. Add device with type

*/

namespace HubPlatform {
namespace TwoGig {
namespace UT {

using namespace ::testing;

struct DevicesRepositoryTestSuite : public testing::Test {
    DevicesRepositoryTestSuite()
    : m_devices( m_database ) {}

    bool isDeviceExistInDB( uint32_t serial ) {
        return m_database.has( Resources::DB::TableNames::Devices,
                               JSON( db::ID_FIELD_NAME << std::to_string( serial ) ) );
    }

    bool addDevice( uint32_t txId ) {
        return m_devices.addDevice( { txId } );
    }

    bool addDevice( uint32_t txId, ::TwoGig::DeviceType::Enum deviceType ) {
        return m_devices.addDevice( { txId }, deviceType );
    }

    bool removeDevice( uint32_t txId ) {
        return m_devices.removeDevice( { txId } );
    }

protected:
    ::HubPlatform::UT::FakeDbClient m_database;
    DevicesRepositoryImpl m_devices;
};

TEST_F( DevicesRepositoryTestSuite, addDevices ) {
    ASSERT_TRUE( addDevice( 1 ) );
    ASSERT_TRUE( addDevice( 2 ) );
    ASSERT_TRUE( addDevice( 3 ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_TRUE( isDeviceExistInDB( 2 ) );
    ASSERT_TRUE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, hasDevices ) {
    ASSERT_TRUE( addDevice( 1 ) );
    ASSERT_TRUE( addDevice( 2 ) );
    ASSERT_TRUE( addDevice( 3 ) );

    ASSERT_TRUE( m_devices.hasDevice( 1 ) );
    ASSERT_TRUE( m_devices.hasDevice( 2 ) );
    ASSERT_TRUE( m_devices.hasDevice( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, removeDevice ) {
    ASSERT_TRUE( addDevice( 1 ) );
    ASSERT_TRUE( addDevice( 2 ) );
    ASSERT_TRUE( addDevice( 3 ) );

    ASSERT_TRUE( removeDevice( 2 ) );
    ASSERT_TRUE( removeDevice( 3 ) );

    ASSERT_TRUE( m_devices.hasDevice( 1 ) );
    ASSERT_FALSE( m_devices.hasDevice( 2 ) );
    ASSERT_FALSE( m_devices.hasDevice( 3 ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_FALSE( isDeviceExistInDB( 2 ) );
    ASSERT_FALSE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, removeAllDevice ) {
    ASSERT_TRUE( addDevice( 1 ) );
    ASSERT_TRUE( addDevice( 2 ) );
    ASSERT_TRUE( addDevice( 3 ) );

    m_devices.reset();

    ASSERT_FALSE( m_devices.hasDevice( 1 ) );
    ASSERT_FALSE( m_devices.hasDevice( 2 ) );
    ASSERT_FALSE( m_devices.hasDevice( 3 ) );

    ASSERT_FALSE( isDeviceExistInDB( 1 ) );
    ASSERT_FALSE( isDeviceExistInDB( 2 ) );
    ASSERT_FALSE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, addExistingDevices ) {
    ASSERT_TRUE( addDevice( 1 ) );

    ASSERT_FALSE( addDevice( 1 ) );
    ASSERT_FALSE( addDevice( 1 ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
}

TEST_F( DevicesRepositoryTestSuite, removeNotExistingDevices ) {
    ASSERT_FALSE( removeDevice( 1 ) );
    ASSERT_FALSE( removeDevice( 1 ) );

    ASSERT_FALSE( isDeviceExistInDB( 1 ) );
}

TEST_F( DevicesRepositoryTestSuite, addDeviceWithMultipleTxIds ) {
    ASSERT_TRUE( m_devices.addDevice( { 1 } ) );
    ASSERT_FALSE( m_devices.addDevice( { 1, 2 } ) );
    ASSERT_TRUE( m_devices.addDevice( { 2, 3 } ) );

    ASSERT_TRUE( isDeviceExistInDB( 1 ) );
    ASSERT_TRUE( isDeviceExistInDB( 2 ) );
    ASSERT_TRUE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, removeDeviceWithMultipleTxIds ) {
    ASSERT_TRUE( m_devices.addDevice( { 1 } ) );
    ASSERT_TRUE( m_devices.addDevice( { 2, 3 } ) );

    ASSERT_FALSE( m_devices.removeDevice( { 1, 2, 3, 4 } ) );
    ASSERT_TRUE( m_devices.removeDevice( { 1, 2 } ) );

    ASSERT_FALSE( m_devices.hasDevice( 1 ) );
    ASSERT_FALSE( m_devices.hasDevice( 2 ) );
    ASSERT_TRUE( m_devices.hasDevice( 3 ) );

    ASSERT_FALSE( isDeviceExistInDB( 1 ) );
    ASSERT_FALSE( isDeviceExistInDB( 2 ) );
    ASSERT_TRUE( isDeviceExistInDB( 3 ) );
}

TEST_F( DevicesRepositoryTestSuite, forEachDevice ) {
    ASSERT_TRUE( m_devices.addDevice( { 1 } ) );
    ASSERT_TRUE( m_devices.addDevice( { 4 } ) );
    ASSERT_TRUE( m_devices.addDevice( { 2, 3 } ) );

    std::vector< uint32_t > nodes;
    m_devices.forEachDevice( [&nodes]( uint32_t txId ) { nodes.push_back( txId ); } );
    ASSERT_EQ( nodes.size(), 4 );
    ASSERT_EQ( nodes.at( 0 ), 1 );
    ASSERT_EQ( nodes.at( 1 ), 2 );
    ASSERT_EQ( nodes.at( 2 ), 3 );
    ASSERT_EQ( nodes.at( 3 ), 4 );
}

TEST_F( DevicesRepositoryTestSuite, addDevicesWithType ) {
    ASSERT_TRUE( addDevice( 1, ::TwoGig::DeviceType::CoDetector ) );
    ASSERT_TRUE( m_devices.addDevice( { 2, 3 }, ::TwoGig::DeviceType::Repeater ) );
    ASSERT_TRUE( addDevice( 4 ) );

    ASSERT_EQ( m_devices.getDeviceType( 1 ), ::TwoGig::DeviceType::CoDetector );
    ASSERT_EQ( m_devices.getDeviceType( 2 ), ::TwoGig::DeviceType::Repeater );
    ASSERT_EQ( m_devices.getDeviceType( 3 ), ::TwoGig::DeviceType::Repeater );
    ASSERT_EQ( m_devices.getDeviceType( 4 ), ::TwoGig::DeviceType::SmokeDetector );
}

}  // namespace UT
}  // namespace TwoGig
}  // namespace HubPlatform
