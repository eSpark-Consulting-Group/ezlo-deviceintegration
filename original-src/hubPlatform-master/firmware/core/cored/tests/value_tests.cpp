#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Bool value
DONE        1.1 Create
DONE            1.1.1 By default
DONE            1.1.2 From bool
DONE            1.1.3 From int
DONE            1.1.4 From different type
DONE        1.2 Update
DONE            1.2.1 By the same value
DONE            1.2.2 By different value
DONE            1.2.3 By different type
DONE    2 Float value
DONE        2.1 Create
DONE            2.1.1 By default
DONE            2.1.2 From float
DONE            2.1.3 From int
DONE            2.1.4 From different type
DONE        2.2 Update
DONE            2.2.1 By the same value
DONE            2.2.2 By different value
DONE            2.2.3 By different type
DONE    3 Int value
DONE        3.1 Create
DONE            3.1.1 By default
DONE            3.1.2 From int
DONE            3.1.3 From different type
DONE        3.2 Update
DONE            3.2.1 By the same value
DONE            3.2.2 By different value
DONE            3.2.3 By different type
DONE    4 Rgb value
DONE        4.1 Create
DONE            4.1.1 By default
DONE            4.1.2 From rgb
DONE            4.1.3 From different type
DONE        4.2 Update
DONE            4.2.1 By the same value
DONE            4.2.2 By different value
DONE            4.2.3 By different type
DONE    5 String value
DONE         5.1 Create
DONE            5.1.1 By default
DONE            5.1.2 From string
DONE            5.1.3 From different type
DONE        5.2 Update
DONE            5.2.1 By the same value
DONE            5.2.2 By different value
DONE            5.2.3 By different type
DONE    6 Camera stream value
DONE        6.1 Create
DONE            6.1.1 By default
DONE            6.1.2 From json
DONE            6.1.3 From different type
DONE        6.2 Update
DONE            6.2.1 By the same value
DONE            6.2.2 By different value
DONE            6.2.3 By different type
DONE    7 Token value
DONE        7.1 Create
DONE           7.1.1 By default
DONE           7.1.2 From json
DONE           7.1.3 from different type
DONE        7.2 Update
DONE            7.2.1 By the same value
DONE            7.2.2 By different value
DONE            7.2.3 By different type
DONE    8 Scalable value
DONE        8.1 Create
DONE            8.1.1 By default
DONE            8.1.2 From json
DONE            8.1.3 From different type
DONE            8.1.4 Compare temperature values
DONE            8.1.5 Compare scalable values
DONE            8.1.6 Compare scalable values with different relativity
DONE            8.1.7 Convert temperature to different scales
DONE    9 Dictionary value
DONE        9.1 Create
DONE            9.1.1 By default
DONE            9.1.2 From json
DONE            9.1.3 From different type
DONE        9.2 Compare
DONE            9.2.1 Compare dictionaries
DONE            9.2.2 Compare dictionary and different type
DONE        9.3 Update
DONE            9.3.1 Edit dictionary
DONE    10 UserCode value
DONE        10.1 Create
DONE           10.1.1 By default
DONE           10.1.2 From json
DONE           10.1.3 from different type
DONE        10.2 Update
DONE            10.2.1 By the same value
DONE            10.2.2 By different value
DONE            10.2.3 By different type
DONE    11 ValueFactory
DONE        11.1 Creation of default values
DONE        11.2 Creation from invalid type
DONE    12 UserLockOperation value
DONE        12.1 Create
DONE            12.1.1 By default
DONE            12.1.2 From json
DONE                12.1.2.1 Valid data
DONE                12.1.2.2 Invalid action value
DONE                12.1.2.3 Invalid id value
DONE            12.1.4 From different type
DONE        12.2 Update
DONE            12.2.1 By the same value
DONE            12.2.2 By different value
DONE            12.2.3 By different type
DONE    13 Array value
DONE        13.1 Create
DONE            13.1.1 By default
DONE            13.1.2 From json
DONE            13.1.3 From different type
DONE        13.2 Compare
DONE            13.2.1 Compare arrays
DONE            13.2.2 Compare array and different type
DONE    14 DailyInterval value
DONE        14.1 Create
DONE           14.1.1 By default
DONE           14.1.2 From json
DONE           14.1.3 from different type
DONE        14.2 Update
DONE            14.2.1 By the same value
DONE            14.2.2 By different value
DONE            14.2.3 By different type
DONE    15 WeeklyInterval value
DONE        15.1 Create
DONE           15.1.1 By default
DONE           15.1.2 From json
DONE           15.1.3 from different type
DONE        15.2 Update
DONE            15.2.1 By the same value
DONE            15.2.2 By different value
DONE            15.2.3 By different type
DONE    16 SoundInfo value
DONE        16.1 Create
DONE           16.1.1 By default
DONE           16.1.2 From json
DONE           16.1.3 from different type
DONE        16.2 Update
DONE            16.2.1 By the same value
DONE            16.2.2 By different value
DONE            16.2.3 By different type
DONE    17 UserCodeAction value
DONE        17.1 Create
DONE            17.1.1 By default
DONE            17.1.2 From json
DONE                17.1.2.1 Valid data
DONE                17.1.2.2 Invalid action value
DONE                17.1.2.3 Invalid id value
DONE            17.1.4 From different type
DONE        17.2 Update
DONE            17.2.1 By the same value
DONE            17.2.2 By different value
DONE            17.2.3 By different type
DONE    18 Action value
DONE        18.1 Create
DONE            18.1.1 By default
DONE            18.1.2 From json
DONE            18.1.3 From different type
DONE        18.2 Update
DONE            18.2.1 By the same value
DONE            18.2.2 By different value
DONE            18.2.3 By different type
DONE    19 ZwaveConfiguration value
DONE        19.1 Create
DONE            19.1.1 By default
DONE            19.1.2 From json
DONE            19.1.3 From different type
DONE            19.1.4 From json without has setter field
DONE        19.2 Update
DONE            19.2.1 By the same value
DONE            19.2.2 By different value
DONE            19.2.3 By different type
DONE    20 DetectableObject value
DONE        20.1 Create
DONE            20.1.1 By default
DONE            20.1.2 From json
DONE            20.1.3 From different type
DONE            20.1.4 From json without last hotzone field
DONE        20.2 Update
DONE            20.2.1 By the same value
DONE            20.2.2 By different value
DONE            20.2.3 By different type
DONE    21 DetectablePerson value
DONE        21.1 Create
DONE            21.1.1 By default
DONE            21.1.2 From json
DONE            21.1.3 From different type
DONE            21.1.4 From json without last hotzone field
DONE        21.2 Update
DONE            21.2.1 By the same value
DONE            21.2.2 By different value
DONE            21.2.3 By different type
DONE    22 DetectableQRCode value
DONE        22.1 Create
DONE            22.1.1 By default
DONE            22.1.2 From json
DONE            22.1.3 From different type
DONE            22.1.4 From json without last hotzone field
DONE        22.2 Update
DONE            22.2.1 By the same value
DONE            22.2.2 By different value
DONE            22.2.3 By different type
DONE    23 CameraHotzone value
DONE        23.1 Create
DONE            23.1.1 By default
DONE            23.1.2 From json
DONE            23.1.3 From different type
DONE        23.2 Update
DONE            23.2.1 By the same value
DONE            23.2.2 By different value
DONE            23.2.3 By different type
DONE    24 HotzoneMatch value
DONE        24.1 Create
DONE            24.1.1 By default
DONE            24.1.2 From json
DONE            24.1.3 From invalid json
DONE            24.1.4 From different type
DONE        24.2 Update
DONE            24.2.1 By the same value
DONE            24.2.2 By different value
DONE            24.2.3 By different type
DONE        24.3 Helper functions
DONE            24.1.3 Test helper functions
DONE    25 Geofence value
DONE        25.1 Create
DONE            25.1.1 By default
DONE            25.1.2 From json
DONE            25.1.3 From different type
DONE        25.2 Update
DONE            25.2.1 By the same value
DONE            25.2.2 By different value
DONE            25.2.3 By different type
DONE    26 ShutterSecondPart value
DONE        26.1 Create
DONE            26.1.1 By default
DONE            26.1.2 From json
DONE                26.1.2.1 Valid data
DONE                26.1.2.2 Invalid action value
DONE                26.1.2.3 Invalid id value
DONE            26.1.4 From different type
DONE        26.2 Update
DONE            26.2.1 By the same value
DONE            26.2.2 By different value
DONE            26.2.3 By different type
DONE    27 AirQuality value
DONE        27.1 Create
DONE            27.1.1 From json
DONE    28 LedSegment value
DONE        28.1 Create
DONE            28.1.1 By default
DONE            28.1.2 From json
DONE                28.1.2.1 Valid data
DONE                28.1.2.2 Invalid name value
DONE                28.1.2.3 Invalid start value
DONE                28.1.2.4 Invalid end value
DONE            28.1.4 From different type
DONE        28.2 Update
DONE            28.2.1 By the same value
DONE            28.2.2 By different value
DONE            28.2.3 By different type
DONE    29 LedEffect value
DONE        29.1 Create
DONE            29.1.1 By default
DONE            29.1.2 From json
DONE                29.1.2.1 Valid data mandatory
DONE                29.1.2.1 Valid data optional
DONE                29.1.2.2 Invalid name value
DONE            29.1.4 From different type
DONE        29.2 Update
DONE            29.2.1 By the same value
DONE            29.2.2 By different value
DONE            29.2.3 By different type
DONE    28 SubstanceAmount value
DONE        28.1 Create
DONE            28.1.1 From json

*/

namespace HubPlatform {
namespace UT {
namespace {
const char* const SomeText{ "some_text" };
}

using namespace ::testing;
using namespace CoreD::Model;

class ValueTestSuite : public Test {
public:
    bool update( const Json& json, Value& value ) {
        auto newValue = ValueFactory::createItemValue( value.getType(), json );
        if( *newValue != value ) {
            value = *newValue;
            return true;
        }

        return false;
    }
};

TEST_F( ValueTestSuite, testBoolValueCreateByDefault /*1.1.1*/ ) {
    auto boolValue = BoolValue::createDefaultValue();

    EXPECT_TRUE( boolValue );

    EXPECT_EQ( boolValue->toJson(), Json( false ) );
    EXPECT_EQ( boolValue->get(), false );
}

TEST_F( ValueTestSuite, testBoolValueCreateFromBool /*1.1.2*/ ) {
    const Json json( true );
    auto boolValue = BoolValue::createValue( json );

    EXPECT_TRUE( boolValue );

    EXPECT_EQ( boolValue->toJson(), json );
    EXPECT_EQ( boolValue->get(), true );
}

TEST_F( ValueTestSuite, testBoolValueCreateFromInt /*1.1.3*/ ) {
    const Json json( 255 );
    auto boolValue = BoolValue::createValue( json );

    EXPECT_TRUE( boolValue );

    EXPECT_EQ( boolValue->toJson(), Json( true ) );
    EXPECT_EQ( boolValue->get(), true );
}

TEST_F( ValueTestSuite, testBoolValueCreateFromDifferentType /*1.1.4*/ ) {
    EXPECT_THROW( BoolValue::createValue( SomeText ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testBoolValueUpdateByTheSameValue /*1.2.1*/ ) {
    const Json json( true );
    auto boolValue = BoolValue::createValue( json );

    EXPECT_FALSE( update( json, *boolValue ) );
    EXPECT_EQ( boolValue->toJson(), Json( true ) );
}

TEST_F( ValueTestSuite, testBoolValueUpdateByDifferentValue /*1.2.2*/ ) {
    const Json json( true );
    auto boolValue = BoolValue::createValue( json );

    EXPECT_TRUE( boolValue );
    const Json newValue( false );

    EXPECT_TRUE( update( newValue, *boolValue ) );
    EXPECT_EQ( boolValue->toJson(), Json( false ) );
}

TEST_F( ValueTestSuite, testBoolValueUpdateByDifferentType /*1.2.3*/ ) {
    const Json json( true );
    auto boolValue = BoolValue::createValue( json );

    EXPECT_TRUE( boolValue );
    const Json newValue( SomeText );

    EXPECT_THROW( update( newValue, *boolValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( boolValue->toJson(), Json( true ) );
}

TEST_F( ValueTestSuite, testFloatValueCreateByDefault /*2.1.1*/ ) {
    constexpr float val{ 0.0f };
    auto floatValue = FloatValue::createDefaultValue();

    EXPECT_TRUE( floatValue );

    EXPECT_EQ( floatValue->toJson(), Json( val ) );
    EXPECT_EQ( floatValue->get(), val );
}

TEST_F( ValueTestSuite, testFloatValueCreateFromFloat /*2.1.2*/ ) {
    constexpr float val{ 10.0f };
    const Json json( val );
    auto floatValue = FloatValue::createValue( json );

    EXPECT_TRUE( floatValue );

    EXPECT_EQ( floatValue->toJson(), json );
    EXPECT_EQ( floatValue->get(), val );
}

TEST_F( ValueTestSuite, testFloatValueCreateFromInt /*2.1.3*/ ) {
    constexpr int val{ 10 };
    const Json json( val );
    auto floatValue = FloatValue::createValue( json );

    EXPECT_TRUE( floatValue );

    EXPECT_EQ( floatValue->toJson(), json );
    EXPECT_EQ( floatValue->get(), val );
}

TEST_F( ValueTestSuite, testFloatValueCreateFromDifferentType /*2.1.4*/ ) {
    EXPECT_THROW( FloatValue::createValue( SomeText ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testFloatValueUpdateByTheSameValue /*2.2.1*/ ) {
    const Json json( 10.0f );
    auto floatValue = FloatValue::createValue( json );

    EXPECT_TRUE( floatValue );

    EXPECT_FALSE( update( json, *floatValue ) );
    EXPECT_EQ( floatValue->toJson(), json );
}

TEST_F( ValueTestSuite, testFloatValueUpdateByDifferentValue /*2.2.2*/ ) {
    const Json json( 10.0f );
    auto floatValue = FloatValue::createValue( json );

    EXPECT_TRUE( floatValue );
    const Json newValue( 25.5f );

    EXPECT_TRUE( update( newValue, *floatValue ) );
    EXPECT_EQ( floatValue->toJson(), newValue );
}

TEST_F( ValueTestSuite, testFloatValueUpdateByDifferentType /*2.2.3*/ ) {
    const Json json( 10.0f );
    auto floatValue = FloatValue::createValue( json );

    EXPECT_TRUE( floatValue );
    const Json newValue( SomeText );

    EXPECT_THROW( update( newValue, *floatValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( floatValue->toJson(), json );
}

TEST_F( ValueTestSuite, testIntValueCreateByDefault /*3.1.1*/ ) {
    auto intValue = IntValue::createDefaultValue();

    EXPECT_TRUE( intValue );

    EXPECT_EQ( intValue->toJson(), Json( 0 ) );
    EXPECT_EQ( intValue->get(), 0 );
}

TEST_F( ValueTestSuite, testIntValueCreateFromInt /*3.1.2*/ ) {
    constexpr int val{ 5 };
    const Json json( val );
    auto intValue = IntValue::createValue( json );

    EXPECT_TRUE( intValue );

    EXPECT_EQ( intValue->toJson(), json );
    EXPECT_EQ( intValue->get(), val );
}

TEST_F( ValueTestSuite, testIntValueCreateFromDifferentType /*3.1.3*/ ) {
    EXPECT_THROW( IntValue::createValue( SomeText ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testIntValueUpdateByTheSameValue /*3.2.1*/ ) {
    const Json json( 5 );
    auto intValue = IntValue::createValue( json );

    EXPECT_TRUE( intValue );

    EXPECT_FALSE( update( json, *intValue ) );
    EXPECT_EQ( intValue->toJson(), json );
}

TEST_F( ValueTestSuite, testIntValueUpdateByDifferentValue /*3.2.2*/ ) {
    const Json json( 5 );
    auto intValue = IntValue::createValue( json );

    EXPECT_TRUE( intValue );
    const Json newValue( 10 );

    EXPECT_TRUE( update( newValue, *intValue ) );
    EXPECT_EQ( intValue->toJson(), newValue );
}

TEST_F( ValueTestSuite, testIntValueUpdateByDifferentType /*3.2.3*/ ) {
    const Json json( 5 );
    auto intValue = IntValue::createValue( json );

    EXPECT_TRUE( intValue );
    const Json newValue( SomeText );

    EXPECT_THROW( update( newValue, *intValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( intValue->toJson(), json );
}

TEST_F( ValueTestSuite, testRgbValueCreateByDefault /*4.1.1*/ ) {
    auto rgbValue = RgbValue::createDefaultValue();

    EXPECT_TRUE( rgbValue );

    EXPECT_EQ( rgbValue->toJson(), JSON_EMPTY_OBJ );
}

TEST_F( ValueTestSuite, testRgbValueCreateFromRgb /*4.1.2*/ ) {
    const JsonObject json = JSON( "red" << 10 << "green" << 15 );
    auto rgbValue = RgbValue::createValue( json );

    EXPECT_TRUE( rgbValue );

    EXPECT_EQ( rgbValue->toJson(), json );
}

TEST_F( ValueTestSuite, testRgbValueCreateFromDifferentType /*4.1.3*/ ) {
    EXPECT_THROW( RgbValue::createValue( SomeText ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testRgbValueUpdateByTheSameValue /*4.2.1*/ ) {
    const JsonObject json = JSON( "red" << 10 << "green" << 15 );
    auto rgbValue = RgbValue::createValue( json );

    EXPECT_TRUE( rgbValue );

    EXPECT_FALSE( update( json, *rgbValue ) );
    EXPECT_EQ( rgbValue->toJson(), json );
}

TEST_F( ValueTestSuite, testRgbValueUpdateByDifferentValue /*4.2.2*/ ) {
    const JsonObject json = JSON( "red" << 10 << "green" << 15 );
    auto rgbValue = RgbValue::createValue( json );

    EXPECT_TRUE( rgbValue );
    const Json newValue = JSON( "red" << 20 << "blue" << 30 );

    EXPECT_TRUE( update( newValue, *rgbValue ) );
    EXPECT_EQ( rgbValue->toJson(), newValue );
}

TEST_F( ValueTestSuite, testRgbValueUpdateByDifferentType /*4.2.3*/ ) {
    const JsonObject json = JSON( "red" << 10 << "green" << 15 );
    auto rgbValue = RgbValue::createValue( json );

    EXPECT_TRUE( rgbValue );
    const Json newValue( SomeText );

    EXPECT_THROW( update( newValue, *rgbValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( rgbValue->toJson(), json );
}

TEST_F( ValueTestSuite, testStringValueCreateByDefault /*5.1.1*/ ) {
    const std::string val{ "" };
    auto stringValue = StringValue::createDefaultValue();

    EXPECT_TRUE( stringValue );

    EXPECT_EQ( stringValue->toJson(), Json( val ) );
    EXPECT_EQ( stringValue->get(), val );
}

TEST_F( ValueTestSuite, testStringValueCreateFromString /*5.1.2*/ ) {
    const std::string val{ SomeText };
    const Json json( val );
    auto stringValue = StringValue::createValue( json );

    EXPECT_TRUE( stringValue );

    EXPECT_EQ( stringValue->toJson(), json );
    EXPECT_EQ( stringValue->get(), val );
}

TEST_F( ValueTestSuite, testStringValueCreateFromDifferentType /*5.1.3*/ ) {
    EXPECT_THROW( StringValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testStringValueUpdateByTheSameValue /*5.2.1*/ ) {
    const Json json( SomeText );
    auto stringValue = StringValue::createValue( json );

    EXPECT_TRUE( stringValue );

    EXPECT_FALSE( update( json, *stringValue ) );
    EXPECT_EQ( stringValue->toJson(), json );
}

TEST_F( ValueTestSuite, testStringValueUpdateByDifferentValue /*5.2.2*/ ) {
    const Json json( SomeText );
    auto stringValue = StringValue::createValue( json );

    EXPECT_TRUE( stringValue );
    const Json newValue( "new_text" );

    EXPECT_TRUE( update( newValue, *stringValue ) );
    EXPECT_EQ( stringValue->toJson(), newValue );
}

TEST_F( ValueTestSuite, testStringValueUpdateByDifferentType /*5.2.3*/ ) {
    const Json json( SomeText );
    auto stringValue = StringValue::createValue( json );

    EXPECT_TRUE( stringValue );
    const Json newValue( 5 );

    EXPECT_THROW( update( newValue, *stringValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( stringValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraStreamValueCreateByDefault /*6.1.1*/ ) {
    auto cameraStreamValue = CameraStreamValue::createDefaultValue();
    EXPECT_TRUE( cameraStreamValue );

    EXPECT_EQ( cameraStreamValue->toJson(),
               JSON( "localRtspUrl"
                     << ""
                     << "cloudLiveStreamUrl"
                     << "" ) );
}

TEST_F( ValueTestSuite, testCameraStreamValueCreateFromJson /*6.1.2*/ ) {
    const JsonObject json = JSON( "localRtspUrl"
                                  << "rtsp://127.0.0.1:554"
                                  << "cloudLiveStreamUrl"
                                  << "rtsps://127.0.0.1:55000" );
    auto cameraStreamValue = CameraStreamValue::createValue( json );

    EXPECT_TRUE( cameraStreamValue );

    EXPECT_EQ( cameraStreamValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraStreamValueCreateFromDifferentType /*6.1.3*/ ) {
    EXPECT_THROW( CameraStreamValue::createValue( SomeText ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testCameraStreamValueUpdateByTheSameValue /*6.2.1*/ ) {
    const JsonObject json = JSON( "localRtspUrl"
                                  << "rtsp://127.0.0.1:554"
                                  << "cloudLiveStreamUrl"
                                  << "rtsps://127.0.0.1:55000" );
    auto cameraStreamValue = CameraStreamValue::createValue( json );

    EXPECT_TRUE( cameraStreamValue );

    EXPECT_FALSE( update( json, *cameraStreamValue ) );
    EXPECT_EQ( cameraStreamValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraStreamValueUpdateByDifferentValue /*6.2.2*/ ) {
    const JsonObject json = JSON( "localRtspUrl"
                                  << "rtsp://127.0.0.1:554"
                                  << "cloudLiveStreamUrl"
                                  << "rtsps://127.0.0.1:55000" );
    auto cameraStreamValue = CameraStreamValue::createValue( json );

    EXPECT_TRUE( cameraStreamValue );
    const Json newValue = JSON( "cloudLiveStreamUrl"
                                << "rtsps://0.0.0.0:55555" );

    const Json expectedValue = JSON( "localRtspUrl"
                                     << "rtsp://127.0.0.1:554"
                                     << "cloudLiveStreamUrl"
                                     << "rtsps://0.0.0.0:55555" );
    EXPECT_TRUE( update( newValue, *cameraStreamValue ) );
    EXPECT_EQ( cameraStreamValue->toJson(), expectedValue );
}

TEST_F( ValueTestSuite, testCameraStreamValueUpdateByDifferentType /*6.2.3*/ ) {
    const JsonObject json = JSON( "localRtspUrl"
                                  << "rtsp://127.0.0.1:554"
                                  << "cloudLiveStreamUrl"
                                  << "rtsps://127.0.0.1:55000" );
    auto cameraStreamValue = CameraStreamValue::createValue( json );

    EXPECT_TRUE( cameraStreamValue );
    const Json newValue( SomeText );

    EXPECT_THROW( update( newValue, *cameraStreamValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( cameraStreamValue->toJson(), json );
}

TEST_F( ValueTestSuite, testTokenValueCreateByDefault /*7.1.1*/ ) {
    const std::string val{ "" };
    auto tokenValue = TokenValue::createDefaultValue();

    EXPECT_TRUE( tokenValue );

    EXPECT_EQ( tokenValue->toJson(), Json( val ) );
}

TEST_F( ValueTestSuite, testTokenValueCreateFromString /*7.1.2*/ ) {
    const std::string val{ SomeText };
    const Json json( val );
    auto tokenValue = TokenValue::createValue( json );

    EXPECT_TRUE( tokenValue );

    EXPECT_EQ( tokenValue->toJson(), json );
}

TEST_F( ValueTestSuite, testTokenValueCreateFromDifferentType /*7.1.3*/ ) {
    EXPECT_THROW( TokenValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testTokenValueUpdateByTheSameValue /*7.2.1*/ ) {
    const Json json( SomeText );
    auto tokenValue = TokenValue::createValue( json );

    EXPECT_TRUE( tokenValue );

    EXPECT_FALSE( update( json, *tokenValue ) );
    EXPECT_EQ( tokenValue->toJson(), json );
}

TEST_F( ValueTestSuite, testTokenValueUpdateByDifferentValue /*7.2.2*/ ) {
    const Json json( SomeText );
    auto tokenValue = TokenValue::createValue( json );

    EXPECT_TRUE( tokenValue );
    const Json newValue( "token" );

    EXPECT_TRUE( update( newValue, *tokenValue ) );
    EXPECT_EQ( tokenValue->toJson(), newValue );
}

TEST_F( ValueTestSuite, testTokenValueUpdateByDifferentType /*7.2.3*/ ) {
    const Json json( SomeText );
    auto tokenValue = TokenValue::createValue( json );

    EXPECT_TRUE( tokenValue );
    const Json newValue( 5 );

    EXPECT_THROW( update( newValue, *tokenValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( tokenValue->toJson(), json );
}

TEST_F( ValueTestSuite, testScalableValueCreateByDefault /*8.1.1*/ ) {
    for( auto type = ValueType::Enum::Illuminance; type < ValueType::Enum::Temperature;
         type = static_cast< ValueType::Enum >( ( static_cast< int >( type ) + 1 ) ) ) {
        auto value = ValueFactory::createDefaultItemValue( type );
        EXPECT_TRUE( value );

        ASSERT_NE( ValueType( type ).getScales().size(), 0 );
        EXPECT_EQ( value->toJson(), JSON( "value" << 0.f << "scale" << *ValueType( type ).getScales().begin() ) );
    }
}

TEST_F( ValueTestSuite, testScalableValueCreateFromJson /*8.1.2*/ ) {
    // Valid temperature
    {
        const JsonObject json = JSON( "value" << 10 << "scale" << ValueScales::Celsius );
        auto temperatureValue = ValueFactory::createItemValue( ValueType::Enum::Temperature, json );

        EXPECT_TRUE( temperatureValue );
        EXPECT_EQ( temperatureValue->toJson(), json );
    }

    // Valid pressure
    {
        const JsonObject json = JSON( "value" << 22 << "scale" << ValueScales::PoundPerSquareInch );
        auto pressureValue = ValueFactory::createItemValue( ValueType::Enum::Pressure, json );

        EXPECT_TRUE( pressureValue );
        EXPECT_EQ( pressureValue->toJson(), json );
    }

    // Valid temperature, relative is true
    {
        const JsonObject json = JSON( "value" << 10 << "scale" << ValueScales::Celsius << "relative" << true );
        auto temperatureValue = ValueFactory::createItemValue( ValueType::Enum::Temperature, json );

        EXPECT_TRUE( temperatureValue );
        EXPECT_EQ( temperatureValue->toJson(), json );
    }

    // Valid temperature, relative is false
    {
        const JsonObject json = JSON( "value" << 10 << "scale" << ValueScales::Fahrenheit );
        auto temperatureValue = ValueFactory::createItemValue( ValueType::Enum::Temperature, json );

        EXPECT_TRUE( temperatureValue );
        EXPECT_EQ( temperatureValue->toJson(), json );
    }

    // Invalid temperature
    {
        const JsonObject json = JSON( "value" << 10 << "scale" << ValueScales::Meter );
        EXPECT_THROW( ValueFactory::createItemValue( ValueType::Enum::Temperature, json ), ScalesNotFoundError );
    }

    // Invalid pressure
    {
        const JsonObject json = JSON( "value" << 44 << "scale" << ValueScales::Meter );
        EXPECT_THROW( ValueFactory::createItemValue( ValueType::Enum::Pressure, json ), ScalesNotFoundError );
    }
}

TEST_F( ValueTestSuite, testScalableValueCreateFromDifferentType /*8.1.3*/ ) {
    EXPECT_THROW( ValueFactory::createItemValue( ValueType::Enum::Temperature, SomeText ),
                  UnexpectedJsonValueFormatError );
    EXPECT_THROW( ValueFactory::createItemValue( ValueType::Enum::Illuminance, "{}" ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testCompareTemperatureValues /*8.1.4*/ ) {
    auto temperatureValueInCelsius = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
    auto temperatureValueInCelsiusWithEps = ValueFactory::createItemValue(
        ValueType::Enum::Temperature,
        JSON( "value" << 10 + std::numeric_limits< float >::epsilon() << "scale" << ValueScales::Celsius ) );
    auto temperatureValueInFahrenheits = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 10.f * 1.8f + 32.f << "scale" << ValueScales::Fahrenheit ) );
    auto anotherTemperatureValueInFahrenheits = ValueFactory::createItemValue(
        ValueType::Enum::Temperature,
        JSON( "value" << 10.f * 1.8f + 32.f + 0.001f << "scale" << ValueScales::Fahrenheit ) );

    ASSERT_TRUE( temperatureValueInCelsius );
    ASSERT_TRUE( temperatureValueInCelsiusWithEps );
    ASSERT_TRUE( temperatureValueInFahrenheits );
    ASSERT_TRUE( anotherTemperatureValueInFahrenheits );

    EXPECT_EQ( *temperatureValueInCelsius, *temperatureValueInCelsiusWithEps );
    EXPECT_EQ( *temperatureValueInFahrenheits, *temperatureValueInFahrenheits );

    EXPECT_NE( *temperatureValueInFahrenheits, *anotherTemperatureValueInFahrenheits );
    EXPECT_NE( *temperatureValueInFahrenheits, *anotherTemperatureValueInFahrenheits );
    EXPECT_NE( *temperatureValueInCelsius, *anotherTemperatureValueInFahrenheits );

    EXPECT_LT( *temperatureValueInCelsius, *anotherTemperatureValueInFahrenheits );
    EXPECT_LT( *temperatureValueInFahrenheits, *anotherTemperatureValueInFahrenheits );

    EXPECT_FALSE( *temperatureValueInCelsius < *temperatureValueInCelsiusWithEps );
}

TEST_F( ValueTestSuite, testCompareScalableValues /*8.1.5*/ ) {
    auto temperatureValueC10 = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
    auto temperatureValueF50 = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, Json{ { "value", 50 }, { "scale", ValueScales::Fahrenheit } } );
    auto powerValueBtuPerHour = ValueFactory::createItemValue(
        ValueType::Enum::Power, JSON( "value" << 1 << "scale" << ValueScales::BtuPerHour ) );
    auto powerValueWatt =
        ValueFactory::createItemValue( ValueType::Enum::Power, JSON( "value" << 1 << "scale" << ValueScales::Watt ) );
    auto powerValueBtuPerHour2 = ValueFactory::createItemValue(
        ValueType::Enum::Power, JSON( "value" << 1.001f << "scale" << ValueScales::BtuPerHour ) );
    auto powerValueWatt2 = ValueFactory::createItemValue( ValueType::Enum::Power,
                                                          JSON( "value" << 1.001f << "scale" << ValueScales::Watt ) );

    ASSERT_TRUE( temperatureValueC10 );
    ASSERT_TRUE( temperatureValueF50 );
    ASSERT_TRUE( powerValueBtuPerHour );
    ASSERT_TRUE( powerValueWatt );
    ASSERT_TRUE( powerValueBtuPerHour2 );
    ASSERT_TRUE( powerValueWatt2 );

    // throw because of different types
    EXPECT_THROW( *temperatureValueC10 < *powerValueBtuPerHour, ValueError );
    EXPECT_THROW( *temperatureValueC10 == *powerValueBtuPerHour, ValueError );

    // throw because of different scales and no converter for this type
    EXPECT_THROW( *powerValueWatt < *powerValueBtuPerHour, ValueError );
    EXPECT_FALSE( *powerValueWatt == *powerValueBtuPerHour );

    EXPECT_NE( *powerValueWatt, *powerValueWatt2 );
    EXPECT_LT( *powerValueWatt, *powerValueWatt2 );

    EXPECT_NE( *powerValueBtuPerHour, *powerValueBtuPerHour2 );
    EXPECT_LT( *powerValueBtuPerHour, *powerValueBtuPerHour2 );

    EXPECT_EQ( *temperatureValueC10, *temperatureValueF50 );
    EXPECT_EQ( *temperatureValueF50, *temperatureValueC10 );
    EXPECT_EQ( *temperatureValueC10, *temperatureValueC10 );
}

TEST_F( ValueTestSuite, testCompareScalableValuesWithDifferentRelativity /*8.1.6*/ ) {
    auto temperatureValueRelative = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 10 << "scale" << ValueScales::Celsius << "relative" << true ) );

    auto temperatureValueNotRelative = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 10 << "scale" << ValueScales::Celsius << "relative" << false ) );

    auto temperatureValueRelative2 = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 11 << "scale" << ValueScales::Celsius << "relative" << true ) );

    auto temperatureValueNotRelative2 = ValueFactory::createItemValue(
        ValueType::Enum::Temperature, JSON( "value" << 11 << "scale" << ValueScales::Celsius << "relative" << false ) );

    ASSERT_TRUE( temperatureValueRelative );
    ASSERT_TRUE( temperatureValueNotRelative );

    EXPECT_NE( *temperatureValueRelative, *temperatureValueNotRelative );
    EXPECT_NE( *temperatureValueRelative, *temperatureValueNotRelative2 );
    EXPECT_NE( *temperatureValueNotRelative, *temperatureValueNotRelative2 );

    EXPECT_EQ( *temperatureValueRelative, *temperatureValueRelative );
    EXPECT_EQ( *temperatureValueNotRelative, *temperatureValueNotRelative );

    EXPECT_LT( *temperatureValueNotRelative, *temperatureValueNotRelative2 );
    EXPECT_LT( *temperatureValueRelative, *temperatureValueRelative2 );
    EXPECT_FALSE( *temperatureValueRelative2 < *temperatureValueRelative );
    EXPECT_FALSE( *temperatureValueNotRelative2 < *temperatureValueNotRelative );

    // cannot compare relative and not relative scalable values
    EXPECT_THROW( *temperatureValueNotRelative < *temperatureValueRelative, ValueError );
    EXPECT_THROW( *temperatureValueRelative < *temperatureValueNotRelative, ValueError );
}

TEST_F( ValueTestSuite, testConvertTemperature /*8.1.7*/ ) {
    // celsius, relative
    {
        auto value = ValueFactory::createItemValue(
            ValueType::Enum::Temperature,
            JSON( "value" << 10 << "scale" << ValueScales::Celsius << "relative" << true ) );

        ASSERT_TRUE( value );

        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Celsius ), 10 );
        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Fahrenheit ), 18 );
    }

    // celsius, not relative
    {
        auto value = ValueFactory::createItemValue(
            ValueType::Enum::Temperature,
            JSON( "value" << 10 << "scale" << ValueScales::Celsius << "relative" << false ) );

        ASSERT_TRUE( value );

        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Celsius ), 10 );
        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Fahrenheit ), 50 );
    }

    // fahrenheit, relative
    {
        auto value = ValueFactory::createItemValue(
            ValueType::Enum::Temperature,
            JSON( "value" << 36 << "scale" << ValueScales::Fahrenheit << "relative" << true ) );

        ASSERT_TRUE( value );

        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Celsius ), 20 );
        EXPECT_FLOAT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Fahrenheit ), 36 );
    }

    // fahrenheit, not relative
    {
        auto value = ValueFactory::createItemValue(
            ValueType::Enum::Temperature,
            JSON( "value" << 50.18f << "scale" << ValueScales::Fahrenheit << "relative" << false ) );

        ASSERT_TRUE( value );

        EXPECT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Celsius ), 10.1f );
        EXPECT_EQ( static_cast< ScalableValue& >( *value ).getConvertedValue( ValueScales::Fahrenheit ), 50.18f );
    }
}

TEST_F( ValueTestSuite, testDictionaryValueCreateByDefault /*9.1.1*/ ) {
    // simple subtype ( dictionary.temparature )
    {
        auto value =
            ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) );
        auto dictionaryValue = dynamic_cast< DictionaryValue* >( value.get() );
        ASSERT_TRUE( dictionaryValue );
        EXPECT_EQ( dictionaryValue->getValuesType(), ValueType::Enum::Temperature );
        EXPECT_EQ( dictionaryValue->toJson(), JSON_EMPTY_OBJ );
    }

    // complicated subtype ( dictionary.dictionary.int )
    {
        auto value = ValueFactory::createDefaultItemValue(
            ValueType::makeDictionaryType( ValueType::makeDictionaryType( ValueType::Enum::Int ) ) );
        auto dictionaryValue = dynamic_cast< DictionaryValue* >( value.get() );
        ASSERT_TRUE( dictionaryValue );

        EXPECT_EQ( dictionaryValue->getValuesType(), ValueType::makeDictionaryType( ValueType::Enum::Int ) );
        EXPECT_EQ( dictionaryValue->toJson(), JSON_EMPTY_OBJ );
    }
}

TEST_F( ValueTestSuite, testDictionaryValueCreateFromJson /*9.1.2*/ ) {
    // simple subtype ( dictionary.temperature )
    {
        auto jsonDictionaryValue = JSON_EMPTY_OBJ;
        jsonDictionaryValue[ "key_1" ] = JSON( "value" << 10 << "scale" << ValueScales::Celsius );
        jsonDictionaryValue[ "key_2" ] = JSON( "value" << 2 << "scale" << ValueScales::Fahrenheit );
        jsonDictionaryValue[ "key_3" ] = JSON( "value" << -50 << "scale" << ValueScales::Fahrenheit );

        auto value = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ),
                                                    jsonDictionaryValue );
        auto dictionaryValue = dynamic_cast< DictionaryValue* >( value.get() );
        ASSERT_TRUE( dictionaryValue );
        EXPECT_EQ( dictionaryValue->getValuesType(), ValueType::Enum::Temperature );

        for( auto it = jsonDictionaryValue.begin(); it != jsonDictionaryValue.end(); ++it ) {
            auto tempValue = ValueFactory::createItemValue( ValueType::Enum::Temperature, it.value() );
            EXPECT_EQ( *dictionaryValue->getValue( it.key() ), *tempValue );
        }

        EXPECT_EQ( dictionaryValue->toJson(), jsonDictionaryValue );
    }

    // complicated subtype ( dictionary.array.dictionary.temperature )
    {
        auto jsonDictionaryValue = JSON_EMPTY_OBJ;
        jsonDictionaryValue[ "key_1" ] = Json::array();

        jsonDictionaryValue[ "key_2" ] = Json::array();
        jsonDictionaryValue[ "key_2" ].push_back( Json() );
        jsonDictionaryValue[ "key_2" ][ 0 ][ "1" ] = JSON( "value" << 10 << "scale" << ValueScales::Celsius );
        jsonDictionaryValue[ "key_2" ][ 0 ][ "2" ] = JSON( "value" << 20 << "scale" << ValueScales::Celsius );

        jsonDictionaryValue[ "key_3" ] = Json::array();
        jsonDictionaryValue[ "key_3" ].push_back( Json() );
        jsonDictionaryValue[ "key_3" ][ 0 ][ "1" ] = JSON( "value" << 30 << "scale" << ValueScales::Celsius );

        jsonDictionaryValue[ "key_3" ].push_back( Json() );
        jsonDictionaryValue[ "key_3" ][ 1 ][ "1" ] = JSON( "value" << 40 << "scale" << ValueScales::Celsius );
        jsonDictionaryValue[ "key_3" ][ 1 ][ "2" ] = JSON( "value" << 50 << "scale" << ValueScales::Celsius );

        auto value = ValueFactory::createItemValue(
            ValueType::makeDictionaryType(
                ValueType::makeArrayType( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) ) ),
            jsonDictionaryValue );
        auto dictionaryValue = dynamic_cast< DictionaryValue* >( value.get() );
        ASSERT_TRUE( dictionaryValue );
        EXPECT_EQ( dictionaryValue->getValuesType(),
                   ValueType::makeArrayType( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) ) );

        for( auto it = jsonDictionaryValue.begin(); it != jsonDictionaryValue.end(); ++it ) {
            auto tempValue = ValueFactory::createItemValue(
                ValueType::makeArrayType( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) ), it.value() );
            EXPECT_EQ( *dictionaryValue->getValue( it.key() ), *tempValue );
        }

        EXPECT_EQ( dictionaryValue->toJson(), jsonDictionaryValue );
    }
}

TEST_F( ValueTestSuite, testDictionaryValueCreateFromDifferentType /*9.1.3*/ ) {
    EXPECT_THROW(
        ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ), SomeText ),
        CoreD::Model::UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testDictionaryValueCompareDictionaries /*9.2.1*/ ) {
    auto jsonDictionaryTempValue = JSON_EMPTY_OBJ;
    jsonDictionaryTempValue[ "key_1" ] = JSON( "value" << 10 << "scale" << ValueScales::Celsius );
    jsonDictionaryTempValue[ "key_2" ] = JSON( "value" << 2 << "scale" << ValueScales::Fahrenheit );
    jsonDictionaryTempValue[ "key_3" ] = JSON( "value" << -50 << "scale" << ValueScales::Fahrenheit );

    auto jsonDictionaryPowerValue = JSON_EMPTY_OBJ;
    jsonDictionaryPowerValue[ "key_1" ] = JSON( "value" << 22 << "scale" << ValueScales::BtuPerHour );
    jsonDictionaryPowerValue[ "key_2" ] = JSON( "value" << 21 << "scale" << ValueScales::BtuPerHour );
    jsonDictionaryPowerValue[ "key_3" ] = JSON( "value" << -2 << "scale" << ValueScales::BtuPerHour );

    // Compare dictionaries with same value types
    {
        auto value1 = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ),
                                                     jsonDictionaryTempValue );
        auto dictionaryValue1 = dynamic_cast< DictionaryValue* >( value1.get() );
        ASSERT_TRUE( dictionaryValue1 );
        auto value2 = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ),
                                                     jsonDictionaryTempValue );
        auto dictionaryValue2 = dynamic_cast< DictionaryValue* >( value2.get() );
        ASSERT_TRUE( dictionaryValue2 );

        EXPECT_EQ( *dictionaryValue1, *dictionaryValue2 );

        dictionaryValue1->removeValue( jsonDictionaryTempValue.begin().key() );
        EXPECT_NE( *dictionaryValue1, *dictionaryValue2 );
    }

    // Compare dictionaries with different value types
    {
        auto value1 = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ),
                                                     jsonDictionaryTempValue );
        auto dictionaryValue1 = dynamic_cast< DictionaryValue* >( value1.get() );
        ASSERT_TRUE( dictionaryValue1 );

        auto value2 = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Power ),
                                                     jsonDictionaryPowerValue );
        auto dictionaryValue2 = dynamic_cast< DictionaryValue* >( value2.get() );
        EXPECT_TRUE( dictionaryValue2 );

        EXPECT_THROW( *dictionaryValue1 == *dictionaryValue2, ValueError );
    }
}

TEST_F( ValueTestSuite, testDictionaryValueCompareDictionaryAndDifferentType /*9.2.2*/ ) {
    auto dictionaryValue =
        ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) );
    EXPECT_TRUE( dictionaryValue );
    auto tokenValue = ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::Token ) );
    EXPECT_TRUE( tokenValue );

    EXPECT_THROW( *dictionaryValue == *tokenValue, ValueError );
}

TEST_F( ValueTestSuite, testDictionaryValueEditDictionary /*9.3.1*/ ) {
    auto value = ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) );
    auto dictionaryValue = dynamic_cast< DictionaryValue* >( value.get() );

    EXPECT_TRUE( dictionaryValue );
    EXPECT_EQ( dictionaryValue->getValuesCount(), 0 );

    dictionaryValue->addValue( "key_1", JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
    dictionaryValue->addValue( "key_2", JSON( "value" << -5 << "scale" << ValueScales::Fahrenheit ) );
    EXPECT_EQ( dictionaryValue->getValuesCount(), 2 );
    EXPECT_NE( dictionaryValue->getValue( "key_1" ), nullptr );
    EXPECT_NE( dictionaryValue->getValue( "key_2" ), nullptr );

    dictionaryValue->removeValue( "key_2" );
    EXPECT_EQ( dictionaryValue->getValuesCount(), 1 );
    EXPECT_NE( dictionaryValue->getValue( "key_1" ), nullptr );
    EXPECT_EQ( dictionaryValue->getValue( "key_2" ), nullptr );

    dictionaryValue->setValue( "key_2",
                               ScalableValue::createValue( ValueType::Enum::Temperature,
                                                           JSON( "value" << 15 << "scale" << ValueScales::Celsius ) ) );
    dictionaryValue->setValue( "key_1", nullptr );
    EXPECT_EQ( dictionaryValue->getValuesCount(), 1 );
    EXPECT_NE( dictionaryValue->getValue( "key_2" ), nullptr );

    EXPECT_THROW( dictionaryValue->setValue( "key_2", ScalableValue::createDefaultValue( ValueType::Enum::Moisture ) ),
                  UnexpectedValueType );
}

TEST_F( ValueTestSuite, testUserCodeValueCreateByDefault /*10.1.1*/ ) {
    using namespace Resources::UsercodeValueFields;

    const auto defaultJson = JSON( UserNameFieldName << "" << UserCodeFieldName << "" );

    auto userCodeValue = UserCodeValue::createDefaultValue();
    EXPECT_TRUE( userCodeValue );
    EXPECT_EQ( userCodeValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testUserCodeValueCreateFromJson /*10.1.2*/ ) {
    using namespace Resources::UsercodeValueFields;

    // without mode
    {
        const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" );

        auto userCodeValue = UserCodeValue::createValue( json );
        EXPECT_TRUE( userCodeValue );
        EXPECT_EQ( userCodeValue->toJson(), json );
    }

    // with mode
    {
        const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" << UserModeFieldName
                                                  << "messaging" );

        auto userCodeValue = UserCodeValue::createValue( json );
        EXPECT_TRUE( userCodeValue );
        EXPECT_EQ( userCodeValue->toJson(), json );
    }
}

TEST_F( ValueTestSuite, testUserCodeValueCreateFromDifferentType /*10.1.3*/ ) {
    EXPECT_THROW( UserCodeValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserCodeValueUpdateByTheSameValue /*10.2.1*/ ) {
    using namespace Resources::UsercodeValueFields;

    // without mode
    {
        const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" );

        auto userCodeValue = UserCodeValue::createValue( json );
        EXPECT_TRUE( userCodeValue );
        EXPECT_FALSE( update( json, *userCodeValue ) );
        EXPECT_EQ( userCodeValue->toJson(), json );
    }

    // with mode
    {
        const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" << UserModeFieldName
                                                  << "locked_out" );

        auto userCodeValue = UserCodeValue::createValue( json );
        EXPECT_TRUE( userCodeValue );
        EXPECT_FALSE( update( json, *userCodeValue ) );
        EXPECT_EQ( userCodeValue->toJson(), json );
    }
}

TEST_F( ValueTestSuite, testUserCodeValueUpdateByDifferentValue /*10.2.2*/ ) {
    using namespace Resources::UsercodeValueFields;

    const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" );
    const auto jsonNew = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "444444" );

    auto userCodeValue = UserCodeValue::createValue( json );
    EXPECT_TRUE( userCodeValue );
    EXPECT_TRUE( update( jsonNew, *userCodeValue ) );
    EXPECT_EQ( userCodeValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testUserCodeValueUpdateByDifferentType /*10.2.3*/ ) {
    using namespace Resources::UsercodeValueFields;

    const auto json = JSON( UserNameFieldName << "test_user" << UserCodeFieldName << "123456" );
    const Json jsonNew( SomeText );

    auto userCodeValue = UserCodeValue::createValue( json );
    EXPECT_TRUE( userCodeValue );
    EXPECT_THROW( update( jsonNew, *userCodeValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( userCodeValue->toJson(), json );
}

TEST_F( ValueTestSuite, createDefaultValuesByFactory /*11.1*/ ) {
    using Type = typename std::underlying_type< ValueType::Enum >::type;

    for( Type type = 0; type < static_cast< Type >( ValueType::Enum::SimpleTypesCount ); ++type ) {
        for( auto valueType : { ValueType( static_cast< ValueType::Enum >( type ) ),
                                ValueType::makeDictionaryType( static_cast< ValueType::Enum >( type ) ) } ) {
            auto item = ValueFactory::createDefaultItemValue( valueType );
            ASSERT_TRUE( item );
            EXPECT_EQ( item->getType(), valueType );
        }
    }
}

TEST_F( ValueTestSuite, createValuesFromInvalidTypeByFactory /*11.2*/ ) {
    EXPECT_THROW( ValueFactory::createDefaultItemValue( ValueType::Enum::NoValue ), UnexpectedValueType );
    EXPECT_THROW( ValueFactory::createDefaultItemValue( ValueType::Enum::Count ), UnexpectedValueType );
    EXPECT_THROW( ValueFactory::createDefaultItemValue( static_cast< ValueType::Enum >( 0xffff ) ),
                  UnexpectedValueType );
    EXPECT_THROW( ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::NoValue ) ),
                  UnexpectedValueType );

    EXPECT_THROW( ValueFactory::createItemValue( ValueType::Enum::NoValue, {} ), UnexpectedValueType );
}

TEST_F( ValueTestSuite, testUserLockOperationValueCreateByDefault /*12.1.1*/ ) {
    using namespace Resources::UserLockOperation;

    const auto defaultJson = JSON( Fields::UserId << InvalidUserCodeId << Fields::Action << Actions::Unknown );

    auto userLockOperationValue = UserLockOperationValue::createDefaultValue();
    EXPECT_TRUE( userLockOperationValue );
    EXPECT_EQ( userLockOperationValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testUserLockOperationValueCreateFromJsonValidData /*12.1.2.1*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Lock );

    auto userLockOperationValue = UserLockOperationValue::createValue( json );
    EXPECT_TRUE( userLockOperationValue );
    EXPECT_EQ( userLockOperationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testUserLockOperationValueCreateFromJsonInvalidActionValue /*12.1.2.2*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << "invalid_value" );

    EXPECT_THROW( UserLockOperationValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserLockOperationValueCreateFromJsonInvalidIdValue /*12.1.2.3*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << "invalid_id" << Fields::Action << Actions::Unknown );

    EXPECT_THROW( UserLockOperationValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserLockOperationValueCreateFromDifferentType /*12.1.4*/ ) {
    EXPECT_THROW( UserLockOperationValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserLockOperationValueUpdateByTheSameValue /*12.2.1*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Unlock );

    auto userLockOperationValue = UserLockOperationValue::createValue( json );
    EXPECT_TRUE( userLockOperationValue );
    EXPECT_FALSE( update( json, *userLockOperationValue ) );
    EXPECT_EQ( userLockOperationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testUserLockOperationValueUpdateByDifferentValue /*12.2.2*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Unknown );
    const auto jsonNew = JSON( Fields::UserId << 2 << Fields::Action << Actions::Unlock );

    auto userLockOperationValue = UserLockOperationValue::createValue( json );
    EXPECT_TRUE( userLockOperationValue );
    EXPECT_TRUE( update( jsonNew, *userLockOperationValue ) );
    EXPECT_EQ( userLockOperationValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testUserLockOperationValueUpdateByDifferentType /*12.2.3*/ ) {
    using namespace Resources::UserLockOperation;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Unknown );
    const Json jsonNew( SomeText );

    auto userLockOperationValue = UserLockOperationValue::createValue( json );
    EXPECT_TRUE( userLockOperationValue );
    EXPECT_THROW( update( jsonNew, *userLockOperationValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( userLockOperationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testArrayValueCreateByDefault /*13.1.1*/ ) {
    // simple subtype ( array.mass )
    {
        auto value = ValueFactory::createDefaultItemValue( ValueType::makeArrayType( ValueType::Enum::Mass ) );
        auto arrayValue = dynamic_cast< ArrayValue* >( value.get() );
        ASSERT_TRUE( arrayValue );
        EXPECT_EQ( arrayValue->getValuesType(), ValueType::Enum::Mass );
        EXPECT_EQ( arrayValue->toJson(), Json::array() );
    }

    // complicated subtype ( array.array.userCode )
    {
        auto value = ValueFactory::createDefaultItemValue(
            ValueType::makeArrayType( ValueType::makeArrayType( ValueType::Enum::UserCode ) ) );
        auto arrayValue = dynamic_cast< ArrayValue* >( value.get() );
        ASSERT_TRUE( arrayValue );
        EXPECT_EQ( arrayValue->getValuesType(), ValueType::makeArrayType( ValueType::Enum::UserCode ) );
        EXPECT_EQ( arrayValue->toJson(), Json::array() );
    }
}

TEST_F( ValueTestSuite, testArrayValueCreateFromJson /*13.1.2*/ ) {
    // create from empty object
    {
        auto jsonEmptyValue = Json::object();
        auto value =
            ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Token ), jsonEmptyValue );
        auto arrayValue = dynamic_cast< ArrayValue* >( value.get() );
        ASSERT_TRUE( arrayValue );
        EXPECT_EQ( arrayValue->getValuesType(), ValueType::Enum::Token );

        ASSERT_EQ( arrayValue->getValuesCount(), 0 );

        EXPECT_EQ( arrayValue->toJson(), Json::array() );
    }

    // simple subtype ( array.temp )
    {
        auto jsonArrayValue = Json::array();
        jsonArrayValue.push_back( JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
        jsonArrayValue.push_back( JSON( "value" << 2 << "scale" << ValueScales::Fahrenheit ) );
        jsonArrayValue.push_back( JSON( "value" << -50 << "scale" << ValueScales::Fahrenheit ) );

        auto value =
            ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ), jsonArrayValue );
        auto arrayValue = dynamic_cast< ArrayValue* >( value.get() );
        ASSERT_TRUE( arrayValue );
        EXPECT_EQ( arrayValue->getValuesType(), ValueType::Enum::Temperature );

        ASSERT_EQ( arrayValue->getValuesCount(), 3 );

        size_t index = 0;
        arrayValue->forEachValue( [&]( const Value& value ) {
            auto tempValue = ValueFactory::createItemValue( ValueType::Enum::Temperature, jsonArrayValue.at( index ) );
            EXPECT_EQ( value, *tempValue );
            ++index;
        } );

        EXPECT_EQ( arrayValue->toJson(), jsonArrayValue );
    }

    // complicated subtype ( array.array.string )
    {
        auto jsonArrayValue = Json::array();
        jsonArrayValue.push_back( Json::array() );
        jsonArrayValue[ 0 ].push_back( "1" );

        jsonArrayValue.push_back( Json::array() );
        jsonArrayValue[ 1 ].push_back( "1" );

        auto value = ValueFactory::createItemValue(
            ValueType::makeArrayType( ValueType::makeArrayType( ValueType::Enum::String ) ), jsonArrayValue );
        auto arrayValue = dynamic_cast< ArrayValue* >( value.get() );

        EXPECT_EQ( arrayValue->getValuesType(), ValueType::makeArrayType( ValueType::Enum::String ) );
        ASSERT_EQ( arrayValue->getValuesCount(), 2 );
        EXPECT_EQ( arrayValue->toJson(), jsonArrayValue );
    }
}

TEST_F( ValueTestSuite, testArrayValueCreateFromDifferentType /*13.1.3*/ ) {
    EXPECT_THROW( ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ), SomeText ),
                  CoreD::Model::UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testArrayValueCompare /*13.2.1*/ ) {
    auto jsonArrayTempValue = Json::array();
    jsonArrayTempValue.push_back( JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
    jsonArrayTempValue.push_back( JSON( "value" << 2 << "scale" << ValueScales::Fahrenheit ) );
    jsonArrayTempValue.push_back( JSON( "value" << -50 << "scale" << ValueScales::Fahrenheit ) );

    auto jsonArrayPowerValue = Json::array();
    jsonArrayPowerValue.push_back( JSON( "value" << 22 << "scale" << ValueScales::BtuPerHour ) );
    jsonArrayPowerValue.push_back( JSON( "value" << 21 << "scale" << ValueScales::BtuPerHour ) );
    jsonArrayPowerValue.push_back( JSON( "value" << -2 << "scale" << ValueScales::BtuPerHour ) );

    // Compare arrays with same value types
    {
        auto value1 = ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ),
                                                     jsonArrayTempValue );
        auto arrayValue1 = dynamic_cast< ArrayValue* >( value1.get() );
        ASSERT_TRUE( arrayValue1 );

        auto value2 = ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ),
                                                     jsonArrayTempValue );
        auto arrayValue2 = dynamic_cast< ArrayValue* >( value2.get() );
        ASSERT_TRUE( arrayValue2 );

        EXPECT_EQ( *arrayValue1, *arrayValue2 );
        EXPECT_EQ( *arrayValue1, *arrayValue2 );

        arrayValue1->addValue( JSON( "value" << 10 << "scale" << ValueScales::Celsius ) );
        EXPECT_NE( *arrayValue1, *arrayValue2 );
    }

    // Compare arrays with different value types
    {
        auto value1 = ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ),
                                                     jsonArrayTempValue );
        auto arrayValue1 = dynamic_cast< ArrayValue* >( value1.get() );
        ASSERT_TRUE( arrayValue1 );

        auto value2 =
            ValueFactory::createItemValue( ValueType::makeArrayType( ValueType::Enum::Power ), jsonArrayPowerValue );
        auto arrayValue2 = dynamic_cast< ArrayValue* >( value2.get() );
        EXPECT_TRUE( arrayValue2 );

        EXPECT_THROW( *arrayValue1 == *arrayValue2, ValueError );
    }
}

TEST_F( ValueTestSuite, testArrayValueCompareArrayAndDifferentType /*13.2.2*/ ) {
    auto arrayTemperatureValue =
        ValueFactory::createDefaultItemValue( ValueType::makeArrayType( ValueType::Enum::Temperature ) );
    EXPECT_TRUE( arrayTemperatureValue );

    auto dictionaryTemperatureValue =
        ValueFactory::createDefaultItemValue( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) );
    EXPECT_TRUE( dictionaryTemperatureValue );

    auto arrayStringValue = ValueFactory::createDefaultItemValue( ValueType::makeArrayType( ValueType::Enum::String ) );
    EXPECT_TRUE( arrayStringValue );

    auto temperatureValue = ValueFactory::createDefaultItemValue( ValueType::Enum::Temperature );

    EXPECT_THROW( *arrayTemperatureValue == *dictionaryTemperatureValue, ValueError );
    EXPECT_THROW( *arrayTemperatureValue == *arrayStringValue, ValueError );
    EXPECT_THROW( *arrayTemperatureValue == *temperatureValue, ValueError );

    EXPECT_THROW( *arrayStringValue == *dictionaryTemperatureValue, ValueError );
    EXPECT_THROW( *arrayStringValue == *temperatureValue, ValueError );
}

TEST_F( ValueTestSuite, testDailyIntervalValueCreateByDefault /*14.1.1*/ ) {
    using namespace Resources::DailyIntervalFields;

    const auto defaultJson = JSON( StartDateTime << "" << StopDateTime << "" );

    auto dailyIntervalValue = DailyIntervalValue::createDefaultValue();
    EXPECT_TRUE( dailyIntervalValue );
    EXPECT_EQ( dailyIntervalValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testDailyIntervalValueCreateFromJson /*14.1.2*/ ) {
    using namespace Resources::DailyIntervalFields;

    const auto json = JSON( StartDateTime << "2020-10-20T10:50:00" << StopDateTime << "2021-10-20T10:50:00" );

    auto dailyIntervalValue = DailyIntervalValue::createValue( json );
    EXPECT_TRUE( dailyIntervalValue );
    EXPECT_EQ( dailyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDilyIntervalValueCreateFromDifferentType /*14.1.3*/ ) {
    EXPECT_THROW( DailyIntervalValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testDilyIntervalValueUpdateByTheSameValue /*14.2.1*/ ) {
    using namespace Resources::DailyIntervalFields;

    const auto json = JSON( StartDateTime << "2020-10-20T11:55:00" << StopDateTime << "2021-11-20T09:50:00" );

    auto dailyIntervalValue = DailyIntervalValue::createValue( json );
    EXPECT_TRUE( dailyIntervalValue );
    EXPECT_FALSE( update( json, *dailyIntervalValue ) );
    EXPECT_EQ( dailyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDailyIntervalValueUpdateByDifferentValue /*14.2.2*/ ) {
    using namespace Resources::DailyIntervalFields;

    const auto json = JSON( StartDateTime << "2020-10-20T11:55:00" << StopDateTime << "2021-11-20T09:50:00" );
    const auto jsonNew = JSON( StartDateTime << "2020-10-20T13:55:00" << StopDateTime << "2021-11-20T09:50:00" );

    auto dailyIntervalValue = DailyIntervalValue::createValue( json );
    EXPECT_TRUE( dailyIntervalValue );
    EXPECT_TRUE( update( jsonNew, *dailyIntervalValue ) );
    EXPECT_EQ( dailyIntervalValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testDailyIntervalValueUpdateByDifferentType /*14.2.3*/ ) {
    using namespace Resources::DailyIntervalFields;

    const auto json = JSON( StartDateTime << "2020-10-20T11:55:00" << StopDateTime << "2021-11-20T09:50:00" );
    const Json jsonNew( SomeText );

    auto dailyIntervalValue = DailyIntervalValue::createValue( json );
    EXPECT_TRUE( dailyIntervalValue );
    EXPECT_THROW( update( jsonNew, *dailyIntervalValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( dailyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueCreateByDefault /*15.1.1*/ ) {
    using namespace Resources::WeeklyIntervalFields;

    const auto defaultJson = JSON( StartTime << "" << StopTime << "" << WeekDays << Json::array() );

    auto weeklyIntervalValue = WeeklyIntervalValue::createDefaultValue();
    EXPECT_TRUE( weeklyIntervalValue );
    EXPECT_EQ( weeklyIntervalValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueCreateFromJson /*15.1.2*/ ) {
    using namespace Resources::WeeklyIntervalFields;

    const auto json = JSON( StartTime << "10:50:00" << StopTime << "12:50:00" << WeekDays
                                      << JSON_ARRAY( "monday"
                                                     << "wednesday" ) );

    auto weeklyIntervalValue = WeeklyIntervalValue::createValue( json );
    EXPECT_TRUE( weeklyIntervalValue );
    EXPECT_EQ( weeklyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueValueCreateFromDifferentType /*15.1.3*/ ) {
    EXPECT_THROW( WeeklyIntervalValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueValueUpdateByTheSameValue /*15.2.1*/ ) {
    using namespace Resources::WeeklyIntervalFields;

    const auto json = JSON( StartTime << "11:55:00" << StopTime << "09:50:00" << WeekDays
                                      << JSON_ARRAY( "saturday"
                                                     << "monday" ) );

    auto weeklyIntervalValue = WeeklyIntervalValue::createValue( json );
    EXPECT_TRUE( weeklyIntervalValue );
    EXPECT_FALSE( update( json, *weeklyIntervalValue ) );
    EXPECT_EQ( weeklyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueUpdateByDifferentValue /*15.2.2*/ ) {
    using namespace Resources::WeeklyIntervalFields;

    const auto json = JSON( StartTime << "11:55:00" << StopTime << "09:50:00" << WeekDays
                                      << JSON_ARRAY( "saturday"
                                                     << "monday" ) );
    const auto jsonNew1 = JSON( StartTime << "11:55:00" << StopTime << "09:50:00" << WeekDays
                                          << JSON_ARRAY( "saturday"
                                                         << "monday"
                                                         << "sunday" ) );
    const auto jsonNew2 = JSON( StartTime << "11:55:00" << StopTime << "09:51:00" << WeekDays
                                          << JSON_ARRAY( "saturday"
                                                         << "monday"
                                                         << "sunday" ) );
    const auto jsonNew3 = JSON( StartTime << "11:51:00" << StopTime << "09:51:00" << WeekDays
                                          << JSON_ARRAY( "saturday"
                                                         << "monday"
                                                         << "sunday" ) );

    auto weeklyIntervalValue = WeeklyIntervalValue::createValue( json );
    EXPECT_TRUE( weeklyIntervalValue );
    EXPECT_TRUE( update( jsonNew1, *weeklyIntervalValue ) );
    EXPECT_TRUE( update( jsonNew2, *weeklyIntervalValue ) );
    EXPECT_TRUE( update( jsonNew3, *weeklyIntervalValue ) );

    EXPECT_EQ( weeklyIntervalValue->toJson(), jsonNew3 );
}

TEST_F( ValueTestSuite, testWeeklyIntervalValueUpdateByDifferentType /*15.2.3*/ ) {
    using namespace Resources::WeeklyIntervalFields;

    const auto json = JSON( StartTime << "11:55:00" << StopTime << "09:50:00" << WeekDays
                                      << JSON_ARRAY( "saturday"
                                                     << "monday" ) );
    const Json jsonNew( SomeText );

    auto weeklyIntervalValue = WeeklyIntervalValue::createValue( json );
    EXPECT_TRUE( weeklyIntervalValue );
    EXPECT_THROW( update( jsonNew, *weeklyIntervalValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( weeklyIntervalValue->toJson(), json );
}

TEST_F( ValueTestSuite, testSoundInfoValueCreateByDefault /*16.1.1*/ ) {
    using namespace Resources::SoundInfoValueFields;

    const auto defaultJson = JSON( Name << "" << Duration << 0 );

    auto soundInfoValue = SoundInfoValue::createDefaultValue();
    EXPECT_TRUE( soundInfoValue );
    EXPECT_EQ( soundInfoValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testSoundInfoValueCreateFromJson /*16.1.2*/ ) {
    using namespace Resources::SoundInfoValueFields;

    const auto json = JSON( Name << "hello" << Duration << 42 );

    auto soundInfoValue = SoundInfoValue::createValue( json );
    EXPECT_TRUE( soundInfoValue );
    EXPECT_EQ( soundInfoValue->toJson(), json );
}

TEST_F( ValueTestSuite, testSoundInfoValueValueCreateFromDifferentType /*16.1.3*/ ) {
    EXPECT_THROW( SoundInfoValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testSoundInfoValueValueUpdateByTheSameValue /*16.2.1*/ ) {
    using namespace Resources::SoundInfoValueFields;

    const auto json = JSON( Name << "hello" << Duration << 42 );

    auto soundInfoValue = SoundInfoValue::createValue( json );
    EXPECT_TRUE( soundInfoValue );
    EXPECT_FALSE( update( json, *soundInfoValue ) );
    EXPECT_EQ( soundInfoValue->toJson(), json );
}

TEST_F( ValueTestSuite, testSoundInfoValueUpdateByDifferentValue /*16.2.2*/ ) {
    using namespace Resources::SoundInfoValueFields;

    const auto json = JSON( Name << "hello" << Duration << 12 );
    const auto jsonNew1 = JSON( Name << "ola" << Duration << 34 );
    const auto jsonNew2 = JSON( Name << "hej" << Duration << 56 );
    const auto jsonNew3 = JSON( Name << "hallo" << Duration << 78 );

    auto soundInfoValue = SoundInfoValue::createValue( json );
    EXPECT_TRUE( soundInfoValue );
    EXPECT_TRUE( update( jsonNew1, *soundInfoValue ) );
    EXPECT_TRUE( update( jsonNew2, *soundInfoValue ) );
    EXPECT_TRUE( update( jsonNew3, *soundInfoValue ) );

    EXPECT_EQ( soundInfoValue->toJson(), jsonNew3 );
}

TEST_F( ValueTestSuite, testSoundInfoValueUpdateByDifferentType /*16.2.3*/ ) {
    using namespace Resources::SoundInfoValueFields;

    const auto json = JSON( Name << "hello" << Duration << 12 );
    const Json jsonNew( SomeText );

    auto soundInfoValue = SoundInfoValue::createValue( json );
    EXPECT_TRUE( soundInfoValue );
    EXPECT_THROW( update( jsonNew, *soundInfoValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( soundInfoValue->toJson(), json );
}

TEST_F( ValueTestSuite, testUserCodeActionValueCreateByDefault /*17.1.1*/ ) {
    using namespace Resources::UserCodeAction;

    const auto defaultJson = JSON( Fields::UserId << InvalidUserCodeId << Fields::Action << Actions::Unknown );

    auto userCodeActionValue = UserCodeActionValue::createDefaultValue();
    EXPECT_TRUE( userCodeActionValue );
    EXPECT_EQ( userCodeActionValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testUserCodeActionValueCreateFromJsonValidData /*17.1.2.1*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::UserCodeEntered );

    auto userCodeActionValue = UserCodeActionValue::createValue( json );
    EXPECT_TRUE( userCodeActionValue );
    EXPECT_EQ( userCodeActionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testUserCodeActionValueCreateFromJsonInvalidActionValue /*17.1.2.2*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << "invalid_value" );

    EXPECT_THROW( UserCodeActionValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserCodeActionValueCreateFromJsonInvalidIdValue /*17.1.2.3*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << "invalid_id" << Fields::Action << Actions::Unknown );

    EXPECT_THROW( UserCodeActionValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserCodeActionValueCreateFromDifferentType /*17.1.4*/ ) {
    EXPECT_THROW( UserCodeActionValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testUserCodeActionValueUpdateByTheSameValue /*17.2.1*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::InvalidUserCodeEntered );

    auto userCodeActionValue = UserCodeActionValue::createValue( json );
    EXPECT_TRUE( userCodeActionValue );
    EXPECT_FALSE( update( json, *userCodeActionValue ) );
    EXPECT_EQ( userCodeActionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testUserCodeActionValueUpdateByDifferentValue /*17.2.2*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Unknown );
    const auto jsonNew = JSON( Fields::UserId << 2 << Fields::Action << Actions::UserCodeEntered );

    auto userCodeActionValue = UserCodeActionValue::createValue( json );
    EXPECT_TRUE( userCodeActionValue );
    EXPECT_TRUE( update( jsonNew, *userCodeActionValue ) );
    EXPECT_EQ( userCodeActionValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testUserCodeActionValueUpdateByDifferentType /*17.2.3*/ ) {
    using namespace Resources::UserCodeAction;

    const auto json = JSON( Fields::UserId << 1 << Fields::Action << Actions::Unknown );
    const Json jsonNew( SomeText );

    auto userCodeActionValue = UserCodeActionValue::createValue( json );
    EXPECT_TRUE( userCodeActionValue );
    EXPECT_THROW( update( jsonNew, *userCodeActionValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( userCodeActionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testActionValueCreateByDefault /*18.1.1*/ ) {
    using namespace Resources::TextElementFields;

    const auto defaultJson = JSON( Text << "" );

    auto actionValue = ActionValue::createDefaultValue();
    EXPECT_TRUE( actionValue );
    EXPECT_EQ( actionValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testActionValueCreateFromJson /*18.1.2*/ ) {
    using namespace Resources::TextElementFields;

    const auto json = JSON( Text << "text_1" << LangTag << "text_2" );

    auto actionValue = ActionValue::createValue( json );
    EXPECT_TRUE( actionValue );
    EXPECT_EQ( actionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testActionValueCreateFromDifferentType /*18.1.3*/ ) {
    EXPECT_THROW( ActionValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testActionValueUpdateByTheSameValue /*18.2.1*/ ) {
    using namespace Resources::TextElementFields;

    const auto json = JSON( Text << "text_1" << LangTag << "text_2" );

    auto actionValue = ActionValue::createValue( json );
    EXPECT_TRUE( actionValue );
    EXPECT_FALSE( update( json, *actionValue ) );
    EXPECT_EQ( actionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testActionValueUpdateByDifferentValue /*18.2.2*/ ) {
    using namespace Resources::TextElementFields;

    const auto json = JSON( Text << "text_1" << LangTag << "text_2" );
    const auto jsonNew = JSON( Text << "text_3" << LangTag << "text_4" );

    auto actionValue = ActionValue::createValue( json );
    EXPECT_TRUE( actionValue );
    EXPECT_TRUE( update( jsonNew, *actionValue ) );
    EXPECT_EQ( actionValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testActionValueUpdateByDifferentType /*18.2.3*/ ) {
    using namespace Resources::TextElementFields;

    const auto json = JSON( Text << "text_1" << LangTag << "text_2" );
    const Json jsonNew( SomeText );

    auto actionValue = ActionValue::createValue( json );
    EXPECT_TRUE( actionValue );
    EXPECT_THROW( update( jsonNew, *actionValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( actionValue->toJson(), json );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueCreateByDefault /*19.1.1*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    const auto defaultJson = JSON( Fields::Name << "" << Fields::Description << "" << Fields::Number << 0
                                                << Fields::Size << 0 << Fields::Value << 0 << Fields::Format << "dec"
                                                << Resources::DeviceSettingFields::SynchronizationStatus << "pending"
                                                << Resources::DeviceSettingFields::HasSetter << true );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createDefaultValue();
    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueCreateFromJson /*19.1.2*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Description << "description" << Fields::Number << 1
                                         << Fields::Size << 4 << Fields::Value << 3 << Fields::Format << "dec"
                                         << Resources::DeviceSettingFields::SynchronizationStatus << "synced"
                                         << Resources::DeviceSettingFields::HasSetter << false );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createValue( json );
    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueCreateFromDifferentType /*19.1.3*/ ) {
    EXPECT_THROW( ZwaveConfigurationValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueCreateFromJsonWithoutHasSetterField /*19.1.4*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    auto json = JSON( Fields::Name << "name" << Fields::Description << "description" << Fields::Number << 1
                                   << Fields::Size << 4 << Fields::Value << 3 << Fields::Format << "dec"
                                   << Resources::DeviceSettingFields::SynchronizationStatus << "synced" );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createValue( json );

    json[ Resources::DeviceSettingFields::HasSetter ] = true;

    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueUpdateByTheSameValue /*19.2.1*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Description << "description" << Fields::Number << 1
                                         << Fields::Size << 4 << Fields::Value << 3 << Fields::Format << "dec"
                                         << Resources::DeviceSettingFields::SynchronizationStatus << "synced"
                                         << Resources::DeviceSettingFields::HasSetter << true );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createValue( json );
    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_FALSE( update( json, *zwaveConfigurationValue ) );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueUpdateByDifferentValue /*19.2.2*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    const auto json = JSON( Fields::Name << "name_1" << Fields::Description << "description_1" << Fields::Number << 1
                                         << Fields::Size << 1 << Fields::Value << 2 << Fields::Format << "dec"
                                         << Resources::DeviceSettingFields::SynchronizationStatus << "synced"
                                         << Resources::DeviceSettingFields::HasSetter << true );
    const auto jsonNew = JSON( Fields::Name << "name_2" << Fields::Description << "description_2" << Fields::Number << 2
                                            << Fields::Size << 3 << Fields::Value << 4 << Fields::Format << "hex"
                                            << Resources::DeviceSettingFields::SynchronizationStatus << "pending"
                                            << Resources::DeviceSettingFields::HasSetter << true );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createValue( json );
    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_TRUE( update( jsonNew, *zwaveConfigurationValue ) );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testZwaveConfigurationValueUpdateByDifferentType /*19.2.3*/ ) {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    const auto json = JSON( Fields::Name << "name_1" << Fields::Description << "description_1" << Fields::Number << 1
                                         << Fields::Size << 1 << Fields::Value << 2 << Fields::Format << "dec"
                                         << Resources::DeviceSettingFields::SynchronizationStatus << "synced"
                                         << Resources::DeviceSettingFields::HasSetter << true );
    const Json jsonNew( SomeText );

    auto zwaveConfigurationValue = ZwaveConfigurationValue::createValue( json );
    EXPECT_TRUE( zwaveConfigurationValue );
    EXPECT_THROW( update( jsonNew, *zwaveConfigurationValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( zwaveConfigurationValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableObjectValueCreateByDefault /*20.1.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto defaultJson = JSON( Fields::Name << "" );

    auto detectableObjectValue = DetectableObjectValue::createDefaultValue();
    EXPECT_TRUE( detectableObjectValue );
    EXPECT_EQ( detectableObjectValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testDetectableObjectValueCreateFromJson /*20.1.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectableObjectValue = DetectableObjectValue::createValue( json );
    EXPECT_TRUE( detectableObjectValue );
    EXPECT_EQ( detectableObjectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableObjectValueCreateFromDifferentType /*20.1.3*/ ) {
    EXPECT_THROW( DetectableObjectValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testDetectableObjectValueCreateFromJsonWithoutLastHotzoneField /*20.1.4*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" );

    auto detectableObjectValue = DetectableObjectValue::createValue( json );

    EXPECT_TRUE( detectableObjectValue );
    EXPECT_EQ( detectableObjectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableObjectValueUpdateByTheSameValue /*20.2.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectableObjectValue = DetectableObjectValue::createValue( json );
    EXPECT_TRUE( detectableObjectValue );
    EXPECT_FALSE( update( json, *detectableObjectValue ) );
    EXPECT_EQ( detectableObjectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableObjectValueUpdateByDifferentValue /*20.2.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name_1" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021_1" << Fields::ItemId << "hotzoneItemId_1"
                                                               << Fields::Id << "hotzoneId_1" ) );

    const auto jsonNew = JSON( Fields::Name << "name_2" << Fields::LastHotzone
                                            << JSON( Fields::Date << "2021_2" << Fields::ItemId << "hotzoneItemId_2"
                                                                  << Fields::Id << "hotzoneId_2" ) );

    auto detectableObjectValue = DetectableObjectValue::createValue( json );
    EXPECT_TRUE( detectableObjectValue );
    EXPECT_TRUE( update( jsonNew, *detectableObjectValue ) );
    EXPECT_EQ( detectableObjectValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testDetectableObjectValueUpdateByDifferentType /*20.2.3*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    const Json jsonNew( SomeText );

    auto detectableObjectValue = DetectableObjectValue::createValue( json );
    EXPECT_TRUE( detectableObjectValue );
    EXPECT_THROW( update( jsonNew, *detectableObjectValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( detectableObjectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectablePersonValueCreateByDefault /*21.1.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto defaultJson = JSON( Fields::Name << "" );

    auto detectablePersonValue = DetectablePersonValue::createDefaultValue();
    EXPECT_TRUE( detectablePersonValue );
    EXPECT_EQ( detectablePersonValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testDetectablePersonValueCreateFromJson /*21.1.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectablePersonValue = DetectablePersonValue::createValue( json );
    EXPECT_TRUE( detectablePersonValue );
    EXPECT_EQ( detectablePersonValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectablePersonValueCreateFromDifferentType /*21.1.3*/ ) {
    EXPECT_THROW( DetectablePersonValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testDetectablePersonValueCreateFromJsonWithoutLastHotzoneField /*21.1.4*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" );

    auto detectablePersonValue = DetectablePersonValue::createValue( json );

    EXPECT_TRUE( detectablePersonValue );
    EXPECT_EQ( detectablePersonValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectablePersonValueUpdateByTheSameValue /*21.2.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectablePersonValue = DetectablePersonValue::createValue( json );
    EXPECT_TRUE( detectablePersonValue );
    EXPECT_FALSE( update( json, *detectablePersonValue ) );
    EXPECT_EQ( detectablePersonValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectablePersonValueUpdateByDifferentValue /*21.2.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name_1" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021_1" << Fields::ItemId << "hotzoneItemId_1"
                                                               << Fields::Id << "hotzoneId_1" ) );

    const auto jsonNew = JSON( Fields::Name << "name_2" << Fields::LastHotzone
                                            << JSON( Fields::Date << "2021_2" << Fields::ItemId << "hotzoneItemId_2"
                                                                  << Fields::Id << "hotzoneId_2" ) );

    auto detectablePersonValue = DetectablePersonValue::createValue( json );
    EXPECT_TRUE( detectablePersonValue );
    EXPECT_TRUE( update( jsonNew, *detectablePersonValue ) );
    EXPECT_EQ( detectablePersonValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testDetectablePersonValueUpdateByDifferentType /*21.2.3*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    const Json jsonNew( SomeText );

    auto detectablePersonValue = DetectablePersonValue::createValue( json );
    EXPECT_TRUE( detectablePersonValue );
    EXPECT_THROW( update( jsonNew, *detectablePersonValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( detectablePersonValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueCreateByDefault /*22.1.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto defaultJson = JSON( Fields::Name << "" << Fields::Value << "" );

    auto detectableQRCodeValue = DetectableQRCodeValue::createDefaultValue();
    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_EQ( detectableQRCodeValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueCreateFromJson /*22.1.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Value << "value" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectableQRCodeValue = DetectableQRCodeValue::createValue( json );
    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_EQ( detectableQRCodeValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueCreateFromDifferentType /*22.1.3*/ ) {
    EXPECT_THROW( DetectableQRCodeValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueCreateFromJsonWithoutLastHotzoneField /*22.1.4*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Value << "value" );

    auto detectableQRCodeValue = DetectableQRCodeValue::createValue( json );

    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_EQ( detectableQRCodeValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueUpdateByTheSameValue /*22.2.1*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Value << "value" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    auto detectableQRCodeValue = DetectableQRCodeValue::createValue( json );
    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_FALSE( update( json, *detectableQRCodeValue ) );
    EXPECT_EQ( detectableQRCodeValue->toJson(), json );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueUpdateByDifferentValue /*22.2.2*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name_1" << Fields::Value << "value_1" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021_1" << Fields::ItemId << "hotzoneItemId_1"
                                                               << Fields::Id << "hotzoneId_1" ) );

    const auto jsonNew = JSON( Fields::Name << "name_2" << Fields::Value << "value_2" << Fields::LastHotzone
                                            << JSON( Fields::Date << "2021_2" << Fields::ItemId << "hotzoneItemId_2"
                                                                  << Fields::Id << "hotzoneId_2" ) );

    auto detectableQRCodeValue = DetectableQRCodeValue::createValue( json );
    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_TRUE( update( jsonNew, *detectableQRCodeValue ) );
    EXPECT_EQ( detectableQRCodeValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testDetectableQRCodeValueUpdateByDifferentType /*22.2.3*/ ) {
    namespace Fields = Resources::DetectableObjectBaseFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Value << "value" << Fields::LastHotzone
                                         << JSON( Fields::Date << "2021" << Fields::ItemId << "hotzoneItemId"
                                                               << Fields::Id << "hotzoneId" ) );

    const Json jsonNew( SomeText );

    auto detectableQRCodeValue = DetectableQRCodeValue::createValue( json );
    EXPECT_TRUE( detectableQRCodeValue );
    EXPECT_THROW( update( jsonNew, *detectableQRCodeValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( detectableQRCodeValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueCreateByDefault /*23.1.1*/ ) {
    namespace Fields = Resources::CameraHotzoneValueFields;

    const auto defaultJson = JSON( Fields::Name << "" << Fields::Detected << JSON_EMPTY_OBJ );

    auto cameraHotzoneValue = CameraHotzoneValue::createDefaultValue();
    EXPECT_TRUE( cameraHotzoneValue );
    EXPECT_EQ( cameraHotzoneValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueCreateFromJson /*23.1.2*/ ) {
    namespace Fields = Resources::CameraHotzoneValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Detected
                                         << JSON( Fields::Objects << JSON_ARRAY( "objectId" ) << Fields::Persons
                                                                  << JSON_ARRAY( "personId" ) << Fields::QRCodes
                                                                  << JSON_ARRAY( "qrCodeId" ) ) );

    auto cameraHotzoneValue = CameraHotzoneValue::createValue( json );
    EXPECT_TRUE( cameraHotzoneValue );
    EXPECT_EQ( cameraHotzoneValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueCreateFromDifferentType /*23.1.3*/ ) {
    EXPECT_THROW( CameraHotzoneValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueUpdateByTheSameValue /*23.2.1*/ ) {
    namespace Fields = Resources::CameraHotzoneValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Detected
                                         << JSON( Fields::Objects << JSON_ARRAY( "objectId" ) << Fields::Persons
                                                                  << JSON_ARRAY( "personId" ) << Fields::QRCodes
                                                                  << JSON_ARRAY( "qrCodeId" ) ) );

    auto cameraHotzoneValue = CameraHotzoneValue::createValue( json );
    EXPECT_TRUE( cameraHotzoneValue );
    EXPECT_FALSE( update( json, *cameraHotzoneValue ) );
    EXPECT_EQ( cameraHotzoneValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueUpdateByDifferentValue /*23.2.2*/ ) {
    namespace Fields = Resources::CameraHotzoneValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Detected
                                         << JSON( Fields::Objects << JSON_ARRAY( "objectId_1" ) << Fields::Persons
                                                                  << JSON_ARRAY( "personId_1" ) << Fields::QRCodes
                                                                  << JSON_ARRAY( "qrCodeId_1" ) ) );

    const auto jsonNew = JSON( Fields::Name << "name" << Fields::Detected
                                            << JSON( Fields::Objects << JSON_ARRAY( "objectId_2" ) << Fields::Persons
                                                                     << JSON_ARRAY( "personId_2" ) << Fields::QRCodes
                                                                     << JSON_ARRAY( "qrCodeId_2" ) ) );

    auto cameraHotzoneValue = CameraHotzoneValue::createValue( json );
    EXPECT_TRUE( cameraHotzoneValue );
    EXPECT_TRUE( update( jsonNew, *cameraHotzoneValue ) );
    EXPECT_EQ( cameraHotzoneValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testCameraHotzoneValueUpdateByDifferentType /*23.2.3*/ ) {
    namespace Fields = Resources::CameraHotzoneValueFields;

    const auto json = JSON( Fields::Name << "name" << Fields::Detected
                                         << JSON( Fields::Objects << JSON_ARRAY( "objectId" ) << Fields::Persons
                                                                  << JSON_ARRAY( "personId" ) << Fields::QRCodes
                                                                  << JSON_ARRAY( "qrCodeId" ) ) );

    const Json jsonNew( SomeText );

    auto cameraHotzoneValue = CameraHotzoneValue::createValue( json );
    EXPECT_TRUE( cameraHotzoneValue );
    EXPECT_THROW( update( jsonNew, *cameraHotzoneValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( cameraHotzoneValue->toJson(), json );
}

TEST_F( ValueTestSuite, testHotzomeMatchValueCreateByDefault /*24.1.1*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;

    const auto defaultJson = JSON( Fields::Match << "any" );

    auto value = HotzoneMatchValue::createDefaultValue();
    ASSERT_TRUE( value );
    EXPECT_EQ( value->toJson(), defaultJson );

    EXPECT_EQ( value->getMatchPolicy(), HotzoneMatchValue::MatchPolicy::Any );
    EXPECT_EQ( value->getMatchList(), HotzoneMatchValue::MatchList{} );
}

TEST_F( ValueTestSuite, testHotzoneMatchValueCreateFromJson /*24.1.2*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;
    ;

    {  // test "any" match
        const auto json = JSON( Fields::Match << "any" );

        auto value = HotzoneMatchValue::createValue( json );
        ASSERT_TRUE( value );
        EXPECT_EQ( value->toJson(), json );

        EXPECT_EQ( value->getMatchPolicy(), HotzoneMatchValue::MatchPolicy::Any );
        EXPECT_EQ( value->getMatchList(), HotzoneMatchValue::MatchList() );
    }

    {
        const auto json = JSON( Fields::Match << "anyOf" << Fields::List
                                              << JSON_ARRAY( "id1"
                                                             << "id2" ) );

        auto value = HotzoneMatchValue::createValue( json );
        ASSERT_TRUE( value );
        EXPECT_EQ( value->toJson(), json );

        EXPECT_EQ( value->getMatchPolicy(), HotzoneMatchValue::MatchPolicy::AnyOf );
        EXPECT_EQ( value->getMatchList(), HotzoneMatchValue::MatchList( { "id1", "id2" } ) );
    }
}

TEST_F( ValueTestSuite, testHotzoneMatchValueCreateFromInvalidJson /*24.1.3*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;
    ;

    {
        const auto json = JSON( Fields::Match << "unknownType" );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError );
    }
    {  // missing match list
        const auto json = JSON( Fields::Match << "anyOf" );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError );
    }
    {
        const auto json = JSON( Fields::Match << "any" << Fields::List << JSON_EMPTY_ARR );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError )
            << "match list not needed";
    }
    {  // empty match list
        const auto json = JSON( Fields::Match << "anyOf" << Fields::List << JSON_EMPTY_ARR );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError );
    }
    {
        const auto json = JSON( Fields::Match << "anyOf" << Fields::List << JSON_ARRAY( "" ) );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError )
            << "invalid id in match list";
    }
    {
        const auto json = JSON( Fields::Match << "anyOf" << Fields::List << Json( "string" ) );
        EXPECT_THROW( HotzoneMatchValue::createValue( json ), UnexpectedJsonValueFormatError )
            << "invaild type of match list";
    }
}

TEST_F( ValueTestSuite, testHotzoneMatchValueCreateFromDifferentType /*24.1.4*/ ) {
    EXPECT_THROW( HotzoneMatchValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testHotzoneMatchValueUpdateByTheSameValue /*24.2.1*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;

    const auto json = JSON( Fields::Match << "noneOf" << Fields::List
                                          << JSON_ARRAY( "id1"
                                                         << "id2" ) );

    auto value = HotzoneMatchValue::createValue( json );
    ASSERT_TRUE( value );
    EXPECT_FALSE( update( json, *value ) );
    EXPECT_EQ( value->toJson(), json );
}

TEST_F( ValueTestSuite, testHotzoneMatchValueUpdateByDifferentValue /*24.2.2*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;

    const auto json = JSON( Fields::Match << "noneOf" << Fields::List
                                          << JSON_ARRAY( "id1"
                                                         << "id2" ) );
    const auto jsonNew = JSON( Fields::Match << "none" );

    auto value = HotzoneMatchValue::createValue( json );
    ASSERT_TRUE( value );
    EXPECT_TRUE( update( jsonNew, *value ) );
    EXPECT_EQ( value->toJson(), jsonNew );

    EXPECT_EQ( value->getMatchPolicy(), HotzoneMatchValue::MatchPolicy::None );
    EXPECT_EQ( value->getMatchList(), HotzoneMatchValue::MatchList() );
}

TEST_F( ValueTestSuite, testHotzoneMatchValueUpdateByDifferentType /*24.2.3*/ ) {
    namespace Fields = Resources::HotzoneMatchValueFields;

    const auto json = JSON( Fields::Match << "noneOf" << Fields::List
                                          << JSON_ARRAY( "id1"
                                                         << "id2" ) );

    const Json jsonNew( SomeText );

    auto value = HotzoneMatchValue::createValue( json );
    ASSERT_TRUE( value );
    EXPECT_THROW( update( jsonNew, *value ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( value->toJson(), json );
}

TEST_F( ValueTestSuite, testHotzoneMatchValueTestHelperFunctions /*24.3.1*/ ) {
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::Any ), "any" );
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::AnyOf ), "anyOf" );
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::AllOf ), "allOf" );
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::OneOf ), "oneOf" );
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::NoneOf ), "noneOf" );
    EXPECT_EQ( HotzoneMatchValue::matchPolicyToStr( HotzoneMatchValue::MatchPolicy::None ), "none" );
}

TEST_F( ValueTestSuite, testGeofenceValueCreateByDefault /*25.1.1*/ ) {
    namespace Fields = Resources::GeofenceValueFields;

    const auto defaultJson = JSON( Fields::Modified << 0 );

    auto geofenceValue = GeofenceValue::createDefaultValue();
    EXPECT_TRUE( geofenceValue );
    EXPECT_EQ( geofenceValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testGeofenceValueCreateFromJson /*25.1.2*/ ) {
    namespace Fields = Resources::GeofenceValueFields;

    const auto json = JSON( Fields::Modified << 1234 );

    auto geofenceValue = GeofenceValue::createValue( json );
    EXPECT_TRUE( geofenceValue );
    EXPECT_EQ( geofenceValue->toJson(), json );
}

TEST_F( ValueTestSuite, testGeofenceValueCreateFromDifferentType /*25.1.3*/ ) {
    EXPECT_THROW( GeofenceValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testGeofenceValueUpdateByTheSameValue /*25.2.1*/ ) {
    namespace Fields = Resources::GeofenceValueFields;

    const auto json = JSON( Fields::Modified << 1234 );

    auto geofenceValue = GeofenceValue::createValue( json );
    EXPECT_TRUE( geofenceValue );
    EXPECT_FALSE( update( json, *geofenceValue ) );
    EXPECT_EQ( geofenceValue->toJson(), json );
}

TEST_F( ValueTestSuite, testGeofenceValueUpdateByDifferentValue /*25.2.2*/ ) {
    namespace Fields = Resources::GeofenceValueFields;

    const auto json = JSON( Fields::Modified << 1234 );
    const auto jsonNew = JSON( Fields::Modified << 5678 );

    auto geofenceValue = GeofenceValue::createValue( json );
    EXPECT_TRUE( geofenceValue );
    EXPECT_TRUE( update( jsonNew, *geofenceValue ) );
    EXPECT_EQ( geofenceValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testGeofenceValueUpdateByDifferentType /*25.2.3*/ ) {
    namespace Fields = Resources::GeofenceValueFields;

    const auto json = JSON( Fields::Modified << 1234 );
    const Json jsonNew( SomeText );

    auto geofenceValue = GeofenceValue::createValue( json );
    EXPECT_TRUE( geofenceValue );
    EXPECT_THROW( update( jsonNew, *geofenceValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( geofenceValue->toJson(), json );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueCreateByDefault /*26.1.1*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto defaultJson =
        JSON( Fields::DeviceId << InvalidDeviceId << Fields::PartFunction << PartFunctions::Unknown );

    auto shutterSecondPartValue = ShutterSecondPartValue::createDefaultValue();
    EXPECT_TRUE( shutterSecondPartValue );
    EXPECT_EQ( shutterSecondPartValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueCreateFromJsonValidData /*26.1.2.1*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << "device_id_1" << Fields::PartFunction << PartFunctions::Front );

    auto shutterSecondPartValue = ShutterSecondPartValue::createValue( json );
    EXPECT_TRUE( shutterSecondPartValue );
    EXPECT_EQ( shutterSecondPartValue->toJson(), json );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueCreateFromJsonInvalidPartFunctionValue /*26.1.2.2*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << "device_id_1" << Fields::PartFunction << "invalid_value" );

    EXPECT_THROW( ShutterSecondPartValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueCreateFromJsonInvalidIdValue /*26.1.2.3*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << 1 << Fields::PartFunction << PartFunctions::Front );

    EXPECT_THROW( ShutterSecondPartValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueCreateFromDifferentType /*26.1.4*/ ) {
    EXPECT_THROW( ShutterSecondPartValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueUpdateByTheSameValue /*26.2.1*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << "device_id_1" << Fields::PartFunction << PartFunctions::Front );

    auto shutterSecondPartValue = ShutterSecondPartValue::createValue( json );
    EXPECT_TRUE( shutterSecondPartValue );
    EXPECT_FALSE( update( json, *shutterSecondPartValue ) );
    EXPECT_EQ( shutterSecondPartValue->toJson(), json );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueUpdateByDifferentValue /*26.2.2*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << "device_id_1" << Fields::PartFunction << PartFunctions::Front );
    const auto jsonNew = JSON( Fields::DeviceId << "device_id_2" << Fields::PartFunction << PartFunctions::Back );

    auto shutterSecondPartValue = ShutterSecondPartValue::createValue( json );
    EXPECT_TRUE( shutterSecondPartValue );
    EXPECT_TRUE( update( jsonNew, *shutterSecondPartValue ) );
    EXPECT_EQ( shutterSecondPartValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testShutterSecondPartValueUpdateByDifferentType /*26.2.3*/ ) {
    using namespace Resources::ShutterSecondPart;

    const auto json = JSON( Fields::DeviceId << "device_id_1" << Fields::PartFunction << PartFunctions::Front );
    const Json jsonNew( SomeText );

    auto shutterSecondPartValue = ShutterSecondPartValue::createValue( json );
    EXPECT_TRUE( shutterSecondPartValue );
    EXPECT_THROW( update( jsonNew, *shutterSecondPartValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( shutterSecondPartValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCompareAirQualityValues /*27.1.1*/ ) {
    auto airQualityValue = ValueFactory::createItemValue( ValueType::Enum::AirQuality,
                                                          JSON( "value" << 10 << "scale" << ValueScales::Iaq ) );
    auto airQualityValueWithEps = ValueFactory::createItemValue(
        ValueType::Enum::AirQuality,
        JSON( "value" << 10 + std::numeric_limits< float >::epsilon() << "scale" << ValueScales::Iaq ) );

    ASSERT_TRUE( airQualityValue );
    ASSERT_TRUE( airQualityValueWithEps );
    EXPECT_EQ( *airQualityValue, *airQualityValueWithEps );

    EXPECT_FALSE( *airQualityValue < *airQualityValueWithEps );
}

TEST_F( ValueTestSuite, testLedSegmentValueCreateByDefault /*28.1.1*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto defaultJson =
        JSON( SegmentNameFieldName << "" << SegmentStartFieldName << 0 << SegmentEndFieldName << 0 );

    auto ledSegmentValue = LedSegmentValue::createDefaultValue();
    EXPECT_TRUE( ledSegmentValue );
    EXPECT_EQ( ledSegmentValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testLedSegmentValueCreateFromJsonValidData /*28.1.2.1*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentStartFieldName << 5 << SegmentEndFieldName << 15 );

    auto ledSegmentValue = LedSegmentValue::createValue( json );
    EXPECT_TRUE( ledSegmentValue );
    EXPECT_EQ( ledSegmentValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedSegmentValueCreateFromJsonInvalidNameValue /*28.1.2.2*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentStartFieldName << 5 << SegmentEndFieldName << 15 );

    EXPECT_THROW( LedSegmentValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedSegmentValueCreateFromJsonInvalidStartValue /*28.1.2.3*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentEndFieldName << 15 );

    EXPECT_THROW( LedSegmentValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedSegmentValueCreateFromJsonInvalidEndValue /*28.1.2.4*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentStartFieldName << 5 );

    EXPECT_THROW( LedSegmentValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedSegmentPartValueCreateFromDifferentType /*28.1.4*/ ) {
    EXPECT_THROW( LedSegmentValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedSegmentValueUpdateByTheSameValue /*28.2.1*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentStartFieldName << 5 << SegmentEndFieldName << 15 );

    auto ledSegmentValue = LedSegmentValue::createValue( json );
    EXPECT_TRUE( ledSegmentValue );
    EXPECT_FALSE( update( json, *ledSegmentValue ) );
    EXPECT_EQ( ledSegmentValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedSegmentPartValueUpdateByDifferentValue /*28.2.2*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentStartFieldName << 5 << SegmentEndFieldName << 15 );
    const auto jsonNew =
        JSON( SegmentNameFieldName << "Name2" << SegmentStartFieldName << 15 << SegmentEndFieldName << 30 );

    auto ledSegmentValue = LedSegmentValue::createValue( json );
    EXPECT_TRUE( ledSegmentValue );
    EXPECT_TRUE( update( jsonNew, *ledSegmentValue ) );
    EXPECT_EQ( ledSegmentValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testLedSegmentPartValueUpdateByDifferentType /*28.2.3*/ ) {
    using namespace Resources::LedSegmentValueFields;

    const auto json = JSON( SegmentNameFieldName << "Name" << SegmentStartFieldName << 5 << SegmentEndFieldName << 15 );
    const Json jsonNew( SomeText );

    auto ledSegmentValue = LedSegmentValue::createValue( json );
    EXPECT_TRUE( ledSegmentValue );
    EXPECT_THROW( update( jsonNew, *ledSegmentValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( ledSegmentValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedEffectValueCreateByDefault /*29.1.1*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto defaultJson = JSON( EffectNameFieldName << "" );

    auto ledEffectValue = LedEffectValue::createDefaultValue();
    EXPECT_TRUE( ledEffectValue );
    EXPECT_EQ( ledEffectValue->toJson(), defaultJson );
}

TEST_F( ValueTestSuite, testLedEffectValueCreateFromJsonValidDataMandatory /*29.1.2.1*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectNameFieldName << "Name" );

    auto ledEffectValue = LedEffectValue::createValue( json );
    EXPECT_TRUE( ledEffectValue );
    EXPECT_EQ( ledEffectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedEffectValueCreateFromJsonValidDataOptional /*29.1.2.2*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectNameFieldName << "Name" << EffectDescriptionFieldName << "Desc"
                                                << EffectHeadpieceUrlFieldName << "http://link" );

    auto ledEffectValue = LedEffectValue::createValue( json );
    EXPECT_TRUE( ledEffectValue );
    EXPECT_EQ( ledEffectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedEffectValueCreateFromJsonInvalidNameValue /*29.1.2.3*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectDescriptionFieldName << "Desc" << EffectHeadpieceUrlFieldName << "http://link" );

    EXPECT_THROW( LedEffectValue::createValue( json ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedEffectPartValueCreateFromDifferentType /*29.1.4*/ ) {
    EXPECT_THROW( LedEffectValue::createValue( 0 ), UnexpectedJsonValueFormatError );
}

TEST_F( ValueTestSuite, testLedEffectValueUpdateByTheSameValue /*29.2.1*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectNameFieldName << "Name" << EffectDescriptionFieldName << "Desc"
                                                << EffectHeadpieceUrlFieldName << "http://link" );

    auto ledEffectValue = LedEffectValue::createValue( json );
    EXPECT_TRUE( ledEffectValue );
    EXPECT_FALSE( update( json, *ledEffectValue ) );
    EXPECT_EQ( ledEffectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testLedEffectPartValueUpdateByDifferentValue /*29.2.2*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectNameFieldName << "Name" << EffectDescriptionFieldName << "Desc"
                                                << EffectHeadpieceUrlFieldName << "http://link" );
    const auto jsonNew = JSON( EffectNameFieldName << "Name2" << EffectDescriptionFieldName << "Desc2"
                                                   << EffectHeadpieceUrlFieldName << "http://link2" );

    auto ledEffectValue = LedEffectValue::createValue( json );
    EXPECT_TRUE( ledEffectValue );
    EXPECT_TRUE( update( jsonNew, *ledEffectValue ) );
    EXPECT_EQ( ledEffectValue->toJson(), jsonNew );
}

TEST_F( ValueTestSuite, testLedEffectPartValueUpdateByDifferentType /*29.2.3*/ ) {
    using namespace Resources::LedEffectValueFields;

    const auto json = JSON( EffectNameFieldName << "Name" << EffectDescriptionFieldName << "Desc"
                                                << EffectHeadpieceUrlFieldName << "http://link" );
    const Json jsonNew( SomeText );

    auto ledEffectValue = LedEffectValue::createValue( json );
    EXPECT_TRUE( ledEffectValue );
    EXPECT_THROW( update( jsonNew, *ledEffectValue ), UnexpectedJsonValueFormatError );
    EXPECT_EQ( ledEffectValue->toJson(), json );
}

TEST_F( ValueTestSuite, testCompareSubstanceAmountValues /*28.1.1*/ ) {
    auto substanceAmountValue = ValueFactory::createItemValue(
        ValueType::Enum::SubstanceAmount, JSON( "value" << 10 << "scale" << ValueScales::ParticlesPerDeciliter ) );
    auto substanceAmountValueWithEps =
        ValueFactory::createItemValue( ValueType::Enum::SubstanceAmount,
                                       JSON( "value" << 10 + std::numeric_limits< float >::epsilon() << "scale"
                                                     << ValueScales::ParticlesPerDeciliter ) );

    ASSERT_TRUE( substanceAmountValue );
    ASSERT_TRUE( substanceAmountValueWithEps );
    EXPECT_EQ( *substanceAmountValue, *substanceAmountValueWithEps );

    EXPECT_FALSE( *substanceAmountValue < *substanceAmountValueWithEps );
}

}  // namespace UT
}  // namespace HubPlatform
