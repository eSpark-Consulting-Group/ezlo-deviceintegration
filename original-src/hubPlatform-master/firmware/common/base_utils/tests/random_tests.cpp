#include "base_utils/random.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace Common {
namespace UT {

class RandomTestSuite : public ::testing::Test {};

TEST_F( RandomTestSuite, singleValue ) {
    Random random( 5, 5 );
    EXPECT_EQ( random(), 5 );
    EXPECT_EQ( random(), 5 );
    EXPECT_EQ( random(), 5 );
    EXPECT_EQ( random(), 5 );
    EXPECT_EQ( random(), 5 );
}

TEST_F( RandomTestSuite, twoNumbersRange ) {
    Random random( 0, 1 );
    auto value = random();
    EXPECT_GE( value, 0 );
    EXPECT_LE( value, 1 );

    value = random();
    EXPECT_GE( value, 0 );
    EXPECT_LE( value, 1 );

    value = random();
    EXPECT_GE( value, 0 );
    EXPECT_LE( value, 1 );

    value = random();
    EXPECT_GE( value, 0 );
    EXPECT_LE( value, 1 );
}

TEST_F( RandomTestSuite, tenNumbersRange ) {
    Random random( 20, 29 );
    auto value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );

    value = random();
    EXPECT_GE( value, 20 );
    EXPECT_LE( value, 29 );
}

}  // namespace UT
}  // namespace Common
