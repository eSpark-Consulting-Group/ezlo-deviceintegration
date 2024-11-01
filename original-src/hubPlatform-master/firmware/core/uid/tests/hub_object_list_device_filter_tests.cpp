#include "bus/base_bus.h"

#include "common_utils/cpp14.h"
#include <gtest/gtest.h>
#include "uid/response_builders/hub_object_list_device_filter.h"

#include <vector>

namespace HubPlatform {
namespace Uid {
namespace UT {

/*

TESTPLAN

DONE    1. DeviceId filtering
DONE        1.1 Request ids for first of existing devices, only one should be found.
DONE        1.2 Request ids for both devices, both should be found.
DONE        1.3 Request ids for two devices, not listed in items list, none should be found.
DONE        1.4 Default: request ids with empty deviceIds filter, all ids should be found.

*/

namespace {

class StubMessage : public rpc::Message {
public:
    StubMessage( JsonObject const& deviceIdsJson ) {
        setParamsField( "deviceIds", deviceIdsJson );
    }
    GenericSenderPtr getNetworkObject() const override {
        return {};
    }
};

using HubObjectListDeviceFilter = HubPlatform::Uid::HubObjectListDeviceFilter;

}  // namespace

TEST( HubItemsListDeviceFilterTest, filterOutIdsForSingleDevice_oneOutOfTwoFound /*1.1*/ ) {
    std::vector< std::string > deviceIdsFilter{ "id_1" };
    JsonObject jsonDeviceIds( deviceIdsFilter );

    StubMessage message( jsonDeviceIds );

    HubObjectListDeviceFilter itemsFilter( message );

    EXPECT_FALSE( itemsFilter.filterDeviceIds( "id_2" ) );
    EXPECT_TRUE( itemsFilter.filterDeviceIds( "id_1" ) );
}

TEST( HubItemsListDeviceFilterTest, filterOutIdsForSingleDevice_bothFound /*1.2*/ ) {
    std::vector< std::string > deviceIdsFilter{ "id_1", "id_2" };
    JsonObject jsonDeviceIds( deviceIdsFilter );

    StubMessage message( jsonDeviceIds );

    HubObjectListDeviceFilter itemsFilter( message );

    EXPECT_TRUE( itemsFilter.filterDeviceIds( "id_2" ) );
    EXPECT_TRUE( itemsFilter.filterDeviceIds( "id_1" ) );
}

TEST( HubItemsListDeviceFilterTest, nonExistentDeviceIdsInFilter_noneFound /*1.3*/ ) {
    std::vector< std::string > deviceIdsFilter{ "id_1", "id_2" };
    JsonObject jsonDeviceIds( deviceIdsFilter );

    StubMessage message( jsonDeviceIds );

    HubObjectListDeviceFilter itemsFilter( message );

    EXPECT_FALSE( itemsFilter.filterDeviceIds( "id_3" ) );
    EXPECT_FALSE( itemsFilter.filterDeviceIds( "id_4" ) );
}

TEST( HubItemsListDeviceFilterTest, emptyParamsDeviceIds_allFound /*1.4*/ ) {
    std::vector< std::string > deviceIdsFilter{};
    JsonObject jsonDeviceIds( deviceIdsFilter );

    StubMessage msg( jsonDeviceIds );

    HubObjectListDeviceFilter itemsFilter( msg );

    EXPECT_TRUE( itemsFilter.filterDeviceIds( "id_1" ) );
    EXPECT_TRUE( itemsFilter.filterDeviceIds( "id_2" ) );
}

}  // namespace UT
}  // namespace Uid
}  // namespace HubPlatform
