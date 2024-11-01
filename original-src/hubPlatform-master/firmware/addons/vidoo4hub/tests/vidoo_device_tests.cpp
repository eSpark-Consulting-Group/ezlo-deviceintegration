#include <gtest/gtest.h>

#include "vidoo4hub/vidoo_device/vidoo_device.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "database_client/dbqueryparser.h"

#include "vidoo4hub/tests/fakes/fake_vidoo4hub_service.h"
#include "vidoo4hub/tests/fakes/fake_cored_remote_interface.h"
#include "vidoo4hub/tests/fakes/fake_core_manager.h"

/*

TESTPLAN

DONE    1. VidooDevice tests
DONE        1.1 Construct new vidoo device
DONE        1.2 Read from database

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class VidooDeviceTestSuite : public FakeVidoo4HubService {
protected:
    Json readTable() {
        using namespace Vidoo4Hub::Resources::DB;
        return getHostService().getDBClient().findAll( TableNames::VidooDeviceDBName, JSON_EMPTY_OBJ, nullptr );
    }
};

TEST_F( VidooDeviceTestSuite, ConstructNewDevice ) /* 1.1 */ {
    using namespace Vidoo4Hub::Resources::DB;
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";

    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );
    const auto vidooDeviceId = vidooDevice.getDeviceId();
    const auto objectListItemId = vidooDevice.getDetectableItemId( DetectableTypes::Type::Object );
    const auto peopleListItemId = vidooDevice.getDetectableItemId( DetectableTypes::Type::Person );
    const auto qrCodeListItemId = vidooDevice.getDetectableItemId( DetectableTypes::Type::QRCode );

    EXPECT_TRUE( !vidooDeviceId.empty() );
    EXPECT_TRUE( !objectListItemId.empty() );
    EXPECT_TRUE( !peopleListItemId.empty() );
    EXPECT_TRUE( !qrCodeListItemId.empty() );

    EXPECT_EQ( core.m_devices.size(), 1 );
    EXPECT_TRUE( core.m_devices.count( vidooDeviceId ) );

    EXPECT_EQ( core.m_items.size(), 3 );
    EXPECT_TRUE( core.m_items.count( objectListItemId ) );
    EXPECT_TRUE( core.m_items.count( peopleListItemId ) );
    EXPECT_TRUE( core.m_items.count( qrCodeListItemId ) );

    auto table = readTable();
    ASSERT_EQ( table.size(), 1 );
    auto& j = table.front();

    EXPECT_EQ( json_get_str( j, FieldNames::vidooDeviceId ), vidooDeviceId );
    EXPECT_EQ( json_get_str( j, FieldNames::objectListItemId ), objectListItemId );
    EXPECT_EQ( json_get_str( j, FieldNames::peopleListItemId ), peopleListItemId );
    EXPECT_EQ( json_get_str( j, FieldNames::qrCodeListItemId ), qrCodeListItemId );
};

TEST_F( VidooDeviceTestSuite, ReadDeviceFromDatabase ) /* 1.2 */ {
    using namespace Vidoo4Hub::Resources::DB;
    const std::string gatewayId = "gatewayId";

    FakeCoreDRemoteInterface initialCore( getHostService() );
    FakeCoreManager initialCoreManager( initialCore );
    VidooDevice initialDevice( getHostService(), initialCoreManager, gatewayId );

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );

    EXPECT_EQ( vidooDevice.getDeviceId(), initialDevice.getDeviceId() );
    EXPECT_EQ( vidooDevice.getDetectableItemId( DetectableTypes::Type::Object ),
               initialDevice.getDetectableItemId( DetectableTypes::Type::Object ) );
    EXPECT_EQ( vidooDevice.getDetectableItemId( DetectableTypes::Type::Person ),
               initialDevice.getDetectableItemId( DetectableTypes::Type::Person ) );
    EXPECT_EQ( vidooDevice.getDetectableItemId( DetectableTypes::Type::QRCode ),
               initialDevice.getDetectableItemId( DetectableTypes::Type::QRCode ) );

    ASSERT_TRUE( core.m_devices.empty() );
    ASSERT_TRUE( core.m_items.empty() );
    EXPECT_EQ( readTable().size(), 1 );
};

}  // namespace UT
}  // namespace HubPlatform
