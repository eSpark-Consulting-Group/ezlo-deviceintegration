#include "cored/model/values/value_type_traits.h"
#include <gtest/gtest.h>

namespace HubPlatform {

namespace CoreD {
namespace Model {
namespace ValueTypeInfo {
namespace UT {

class ValueTypeIsLess : public ::testing::Test {
public:
    static void hasIsLess( const std::string& valueType, int line );
    static void hasNoIsLess( const std::string& valueType, int line );
};

void ValueTypeIsLess::hasIsLess( const std::string& valueTypeName, int line ) {
    using namespace HubPlatform::CoreD;
    auto valueType = ValueType::fromString( valueTypeName );
    EXPECT_TRUE( isValueTypeValid( valueType ) )
        << "Should be valid type: '" << valueTypeName << "', at: " << __FILE__ << ":" << line;
    EXPECT_TRUE( Model::hasIsLess( valueType ) ) << "Should have isLess/operator <(), at: " << __FILE__ << ":" << line;
}

void ValueTypeIsLess::hasNoIsLess( const std::string& valueTypeName, int line ) {
    using namespace HubPlatform::CoreD;
    auto valueType = ValueType::fromString( valueTypeName );
    EXPECT_TRUE( isValueTypeValid( valueType ) )
        << "Should be valid type: '" << valueTypeName << "', at: " << __FILE__ << ":" << line;
    EXPECT_FALSE( Model::hasIsLess( valueType ) ) << "Should have isLess/operator <(), at: " << __FILE__ << ":" << line;
}

TEST_F( ValueTypeIsLess, hasIsLess ) {
    hasIsLess( "string", __LINE__ );
    hasIsLess( "int", __LINE__ );
    hasIsLess( "float", __LINE__ );
    hasIsLess( "temperature", __LINE__ );
}

TEST_F( ValueTypeIsLess, hasNoIsLess ) {
    hasNoIsLess( "bool", __LINE__ );
    hasNoIsLess( "token", __LINE__ );
    hasNoIsLess( "rgb", __LINE__ );
    hasNoIsLess( "cameraStream", __LINE__ );
    hasNoIsLess( "userCode", __LINE__ );
    hasNoIsLess( "weekly_interval", __LINE__ );
    hasNoIsLess( "daily_interval", __LINE__ );
    hasNoIsLess( "button_state", __LINE__ );
    hasNoIsLess( "user_lock_operation", __LINE__ );
    hasNoIsLess( "user_code_action", __LINE__ );
    hasNoIsLess( "sound_info", __LINE__ );

    hasNoIsLess( "dictionary.token", __LINE__ );
    hasNoIsLess( "array.token", __LINE__ );
}

}  // namespace UT
}  // namespace ValueTypeInfo
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform