#include "base_utils/range.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class RangeTest : public testing::Test {};

TEST_F( RangeTest, includedRanges ) {
    Range< int > range = { 5, 10, RangeType::Included };
    EXPECT_TRUE( range.isIntersection( { 4, 11, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 6, 9, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 10, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 10, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 15, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 15, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 7, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 7, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 5, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 7, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 10, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 5, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 0, 4, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 11, 20, RangeType::Included } ) );
}

TEST_F( RangeTest, excludedRangeAndIncludedRange ) {
    Range< int > range = { 5, 10, RangeType::Excluded };
    EXPECT_TRUE( range.isIntersection( { 4, 11, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 6, 9, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 5, 10, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 7, 10, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 15, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 15, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 5, 7, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 7, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 5, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 7, 7, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 10, 10, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 5, 5, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 4, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 11, 20, RangeType::Included } ) );
}

TEST_F( RangeTest, includeRangeAndExcludedRange ) {
    Range< int > range = { 5, 10, RangeType::Included };
    EXPECT_FALSE( range.isIntersection( { 4, 11, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 6, 9, RangeType::Excluded } ) );
    EXPECT_FALSE( range.isIntersection( { 5, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 15, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 15, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 5, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 5, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 4, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 11, 20, RangeType::Excluded } ) );
}

TEST_F( RangeTest, excludedRanges ) {
    Range< int > range = { 5, 10, RangeType::Excluded };
    EXPECT_TRUE( range.isIntersection( { 4, 11, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 6, 9, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 15, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 15, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 5, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 7, 7, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 10, 10, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 5, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 0, 4, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 11, 20, RangeType::Excluded } ) );
}

TEST_F( RangeTest, singleNumberRange ) {
    Range< int > range = { 5, 5, RangeType::Included };
    EXPECT_TRUE( range.isIntersection( { 5, 5, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 4, 9, RangeType::Included } ) );
    EXPECT_FALSE( range.isIntersection( { 5, 5, RangeType::Excluded } ) );
    EXPECT_FALSE( range.isIntersection( { 4, 9, RangeType::Excluded } ) );
    range = { 5, 5, RangeType::Excluded };
    EXPECT_FALSE( range.isIntersection( { 5, 5, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 4, 9, RangeType::Included } ) );
    EXPECT_TRUE( range.isIntersection( { 5, 5, RangeType::Excluded } ) );
    EXPECT_TRUE( range.isIntersection( { 4, 9, RangeType::Excluded } ) );
}
