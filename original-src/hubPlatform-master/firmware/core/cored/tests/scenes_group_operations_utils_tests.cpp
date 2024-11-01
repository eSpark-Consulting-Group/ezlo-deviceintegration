#include "cored/tests/fixtures/group_operations_fixture.h"
#include "cored/group_operations/device_group_updater.h"
#include "cored/group_operations/group_operations_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Compare string arrays
 2. Compare object arrays
*/

class ScenesGroupOperationsUtilsTests : public ::testing::Test {};

TEST_F( ScenesGroupOperationsUtilsTests, compareStringArrays ) {  // 1
    using namespace HubPlatform::CoreD::GroupOperationsUtils;

    {  // same sizes, diff data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "2", "3" );
        ASSERT_FALSE( areStringArraysEqual( left, right ) );
    }
    {  // diff sizes, diff data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "1", "2", "3" );
        ASSERT_FALSE( areStringArraysEqual( left, right ) );
    }
    {  // same sizes, same data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "1", "2" );
        ASSERT_TRUE( areStringArraysEqual( left, right ) );
    }
    {  // same sizes, same data in different order
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "2", "1" );
        ASSERT_TRUE( areStringArraysEqual( left, right ) );
    }
    {  // empty, non-empty
        auto left = JSON_EMPTY_ARR;
        auto right = JSON_ARRAY_( "1", "2" );
        ASSERT_FALSE( areStringArraysEqual( left, right ) );
    }
    {  // non-empty, empty
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_EMPTY_ARR;
        ASSERT_FALSE( areStringArraysEqual( left, right ) );
    }
    {  // empty, empty
        auto left = JSON_EMPTY_ARR;
        auto right = JSON_EMPTY_ARR;
        ASSERT_TRUE( areStringArraysEqual( left, right ) );
    }
}

TEST_F( ScenesGroupOperationsUtilsTests, compareObjectArrays ) {  // 2
    using namespace HubPlatform::CoreD::GroupOperationsUtils;

    Json updated;
    {  // same sizes, different data deep in object
        Json category1 = R"({ "category": "one", "subcategories": [ "sub-one", "sub-two" ] })"_json;
        Json category2 = R"({ "category": "one", "subcategories": [ "sub-one", "sub-three" ] })"_json;
        Json category3 = R"({ "category": "two", "subcategories": [ "sub-one", "sub-three" ] })"_json;
        auto left = JSON_ARRAY_( category1, category2 );
        auto right = JSON_ARRAY_( category1, category3 );
        ASSERT_FALSE( areObjectArraysEqual( left, right ) );
    }

    {  // same sizes, diff data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "2", "3" );
        ASSERT_FALSE( areObjectArraysEqual( left, right ) );
    }
    {  // diff sizes, diff data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "1", "2", "3" );
        ASSERT_FALSE( areObjectArraysEqual( left, right ) );
    }
    {  // same sizes, same data
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "1", "2" );
        ASSERT_TRUE( areObjectArraysEqual( left, right ) );
    }
    {  // same sizes, same data in different order
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_ARRAY_( "2", "1" );
        ASSERT_TRUE( areObjectArraysEqual( left, right ) );
    }
    {  // empty, non-empty
        auto left = JSON_EMPTY_ARR;
        auto right = JSON_ARRAY_( "1", "2" );
        ASSERT_FALSE( areObjectArraysEqual( left, right ) );
    }
    {  // non-empty, empty
        auto left = JSON_ARRAY_( "1", "2" );
        auto right = JSON_EMPTY_ARR;
        ASSERT_FALSE( areObjectArraysEqual( left, right ) );
    }
    {  // empty, empty
        auto left = JSON_EMPTY_ARR;
        auto right = JSON_EMPTY_ARR;
        ASSERT_TRUE( areObjectArraysEqual( left, right ) );
    }
}

}  // namespace UT
}  // namespace HubPlatform