#include "scenes_manager/scenes_engine/value_source_key.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

 TEST-PLAN

 DONE 1. serialize device item group ids
 DONE    1.1 pack device item group ids
 DONE    1.2 pack device item group ids

 */

class ValueSourceKeyTest : public ::testing::Test {};

TEST_F( ValueSourceKeyTest, packDeviceItemGroupIds ) /* 1.1 */ {
    const std::string deviceGroupId = "deviceGroup";
    const std::string itemGroupId = "itemGroup";

    auto packedValue = ValueSourceKey::packDeviceItemGroupIds( deviceGroupId, itemGroupId );
    const char* const delimiter = "/";
    ASSERT_EQ( packedValue, deviceGroupId + delimiter + itemGroupId );
}

TEST_F( ValueSourceKeyTest, unpackDeviceItemGroupIds ) /* 1.2 */ {
    const std::string deviceGroupId = "deviceGroup";
    const std::string itemGroupId = "itemGroup";

    const char* const delimiter = "/";
    std::string packed = deviceGroupId + delimiter + itemGroupId;

    auto unpackedValue = ValueSourceKey::unpackDeviceItemGroupIds( packed );
    ASSERT_EQ( unpackedValue.first, deviceGroupId );
    ASSERT_EQ( unpackedValue.second, itemGroupId );
}

}  // namespace UT
}  // namespace SceneEngine
