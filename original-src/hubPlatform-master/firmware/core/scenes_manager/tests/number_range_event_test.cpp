#include "scenes_manager/scenes_engine/events/compare_number_range_event.h"
#include "scenes_manager/scenes_engine/events/event_base.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class CompareNumberRangeEventTests : public testing::Test {
public:
    template < typename ValueType >
    static bool testCompareRange( ValueType start, ValueType end, ValueType val, std::string& comparator ) {
        SceneEngine::ItemEventBase::ItemValue startValue;
        SceneEngine::ItemEventBase::ItemValue endValue;
        SceneEngine::ItemEventBase::ItemValue value;

        startValue.setPayload( start );
        endValue.setPayload( end );
        value.setPayload( val );
        CompareNumberRangeEvent event( startValue, endValue, comparator );
        return event.compare( value );
    }

    using ScalableValue = std::pair< Json, std::string >;
    static bool testCompareRange( const ScalableValue& start,
                                  const ScalableValue& end,
                                  const ScalableValue& val,
                                  std::string& comparator ) {
        SceneEngine::ItemEventBase::ItemValue startValue;
        SceneEngine::ItemEventBase::ItemValue endValue;
        SceneEngine::ItemEventBase::ItemValue value;

        startValue = makeScalable( start.first, start.second );
        endValue = makeScalable( end.first, end.second );
        value = makeScalable( val.first, val.second );
        CompareNumberRangeEvent event( startValue, endValue, comparator );
        return event.compare( value );
    }

    template < typename ValueType >
    static SceneEngine::ItemEventBase::ItemValue makeScalable( ValueType value, const std::string& scale ) {
        return makeScalable( ScalableValue{ value, scale } );
    }

    static SceneEngine::ItemEventBase::ItemValue makeScalable( ScalableValue scalable ) {
        SceneEngine::ItemEventBase::ItemValue wrapper;
        if( !scalable.second.empty() )
            wrapper.setPayload( { { "value", { { "value", scalable.first }, { "scale", scalable.second } } } } );
        else
            wrapper.setPayload( { { "value", scalable.first } } );
        return wrapper;
    }
};

TEST_F( CompareNumberRangeEventTests, testCompareBetween ) {
    std::string comparator = "between";

    ASSERT_TRUE( testCompareRange( 10, 50, 30, comparator ) );
    ASSERT_TRUE( testCompareRange( 10, 50, 10, comparator ) );
    ASSERT_TRUE( testCompareRange( 10, 50, 50, comparator ) );
    ASSERT_FALSE( testCompareRange( 10, 50, 60, comparator ) );
    ASSERT_FALSE( testCompareRange( 10, 50, 5, comparator ) );

    ASSERT_TRUE( testCompareRange( 10.3, 50.4, 30.7, comparator ) );
    ASSERT_TRUE( testCompareRange( 10.3, 50.4, 10.3, comparator ) );
    ASSERT_TRUE( testCompareRange( 10.3, 50.4, 50.4, comparator ) );
    ASSERT_FALSE( testCompareRange( 10.3, 50.4, 60.4, comparator ) );
    ASSERT_FALSE( testCompareRange( 10.3, 50.4, 5.2, comparator ) );
}

TEST_F( CompareNumberRangeEventTests, testCompareNotBetween ) {
    std::string comparator = "not_between";
    ASSERT_FALSE( testCompareRange( 10, 50, 10, comparator ) );
    ASSERT_TRUE( testCompareRange( 10, 50, 3, comparator ) );
    ASSERT_FALSE( testCompareRange( 10, 50, 50, comparator ) );
    ASSERT_TRUE( testCompareRange( 10, 50, 60, comparator ) );
    ASSERT_FALSE( testCompareRange( 10, 50, 30, comparator ) );

    ASSERT_TRUE( testCompareRange( 10.3, 50.4, 3.9, comparator ) );
    ASSERT_FALSE( testCompareRange( 10.3, 50.4, 10.3, comparator ) );
    ASSERT_TRUE( testCompareRange( 10.3, 50.4, 60.5, comparator ) );
    ASSERT_FALSE( testCompareRange( 10.3, 50.4, 10.3, comparator ) );
    ASSERT_FALSE( testCompareRange( 10.3, 50.4, 30.8, comparator ) );
}

TEST_F( CompareNumberRangeEventTests, compareBetweenWithScale ) {
    std::string comparator = "between";

    const std::string celsius = "celsius";
    const std::string fahrenheit = "fahrenheit";
    // 0 C = 32 F, 100 C == 212 F
    ASSERT_TRUE( testCompareRange( { 0, celsius }, { 100, celsius }, { 30, celsius }, comparator ) );
    ASSERT_TRUE( testCompareRange( { 0, celsius }, { 100, celsius }, { 32, fahrenheit }, comparator ) );
    ASSERT_TRUE( testCompareRange( { 0, celsius }, { 100, celsius }, { 212, fahrenheit }, comparator ) );
    ASSERT_FALSE( testCompareRange( { 0, celsius }, { 100, celsius }, { 213, fahrenheit }, comparator ) );
    ASSERT_FALSE( testCompareRange( { 0, celsius }, { 100, celsius }, { 31, fahrenheit }, comparator ) );

    ASSERT_TRUE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 30.0, celsius }, comparator ) );
    ASSERT_TRUE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 32.2, fahrenheit }, comparator ) );
    ASSERT_TRUE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 212.0, fahrenheit }, comparator ) );
    ASSERT_FALSE( testCompareRange( { 0.1, celsius }, { 100, celsius }, { 213.0, fahrenheit }, comparator ) );
    ASSERT_FALSE( testCompareRange( { 0.1, celsius }, { 100, celsius }, { 31.0, fahrenheit }, comparator ) );
}

TEST_F( CompareNumberRangeEventTests, compareNotBetweenWithScale ) {
    std::string comparator = "not_between";

    const std::string celsius = "celsius";
    const std::string fahrenheit = "fahrenheit";
    // 0 C = 32 F, 100 C == 212 F
    EXPECT_FALSE( testCompareRange( { 0, celsius }, { 100, celsius }, { 30, celsius }, comparator ) );
    EXPECT_FALSE( testCompareRange( { 0, celsius }, { 100, celsius }, { 32, fahrenheit }, comparator ) );
    EXPECT_FALSE( testCompareRange( { 0, celsius }, { 100, celsius }, { 212, fahrenheit }, comparator ) );
    EXPECT_TRUE( testCompareRange( { 0, celsius }, { 100, celsius }, { 213, fahrenheit }, comparator ) );
    EXPECT_TRUE( testCompareRange( { 0, celsius }, { 100, celsius }, { 31, fahrenheit }, comparator ) );

    EXPECT_FALSE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 30.0, celsius }, comparator ) );
    EXPECT_FALSE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 32.2, fahrenheit }, comparator ) );
    EXPECT_FALSE( testCompareRange( { 0.1, celsius }, { 100.0, celsius }, { 212.0, fahrenheit }, comparator ) );
    EXPECT_TRUE( testCompareRange( { 0.1, celsius }, { 100, celsius }, { 213.0, fahrenheit }, comparator ) );
    EXPECT_TRUE( testCompareRange( { 0.1, celsius }, { 100, celsius }, { 31.0, fahrenheit }, comparator ) );
}

}  // namespace UT
}  // namespace SceneEngine
