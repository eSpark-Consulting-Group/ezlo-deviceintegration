#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/model/values/value_type.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

 TEST PLAN

 DONE   1 Compare values with operators == and <
 DONE       1.1 Compare string
 DONE   2 Compare values with operators == and without <
 DONE       2.1 Compare cameraStream
 DONE   3 type checking
 DONE       3.1 fixed value type differs (has valueType != constant.type)

*/

class CompareValuesTests : public SceneManagerTestsFixture {
public:
    void SetUp() override;
    void TearDown() override;
};

void CompareValuesTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void CompareValuesTests::TearDown() {
    clearExistingDevices( __LINE__ );
}

Json generateCompareValuesBlock( std::string itemId,
                                 std::string comparator,
                                 std::string valueType,
                                 std::string type,
                                 const Json& value ) {
    auto comparisonBlock = CommonNmaTests::replaceTokens( compareValuesWhenBlock,
                                                          { { "__SOURCE_NAME__", "item" },
                                                            { "__SOURCE_TYPE__", "item" },
                                                            { "__SOURCE_VALUE__", "\"" + itemId + "\"" },
                                                            { "__COMPARATOR__", comparator },
                                                            { "__VALUE_TYPE_VALUE__", valueType },
                                                            { "__TYPE__", type },
                                                            { "__UNQUOTED_VALUE__", value.dump() } } );
    return comparisonBlock;
}

TEST_F( CompareValuesTests, valueTypeWithLess_string ) /* 1.1 */ {
    using namespace CoreD::Model::Resources;

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    std::string itemId = device.m_itemId;

    {
        Json scene = Tests::createEmptyScene;
        const char* ValueLow = "low";
        auto comparisonBlock = generateCompareValuesBlock( itemId, "==", "string", "string", ValueLow );
        scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

        SceneID sceneId = createScene( scene );

        setItemValue( makeRemote(), itemId, ValueLow, __LINE__ );

        testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
        testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

        deleteScene( sceneId, __LINE__ );
    }
    {
        Json scene = Tests::createEmptyScene;
        std::string valueModerate = "moderate";
        // Test : "moderate2" > "moderate1"
        auto comparisonBlock = generateCompareValuesBlock( itemId, ">", "string", "string", valueModerate + "0" );
        scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

        SceneID sceneId = createScene( scene );

        setItemValue( makeRemote(), itemId, valueModerate + "1", __LINE__ );

        testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
        testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

        deleteScene( sceneId, __LINE__ );
    }
}

TEST_F( CompareValuesTests, valueTypeNoLessCameraStream ) /* 2.1 */ {
    using namespace CoreD::Model::Resources;
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::CameraStream, __LINE__ );
    std::string itemId = device.m_itemId;

    {  // valueType with operator ==, but without operator<
        Json scene = Tests::createEmptyScene;
        Json valueConstant = { { "localRtspUrl", "rtsp://127.0.0.1:554" }, { "cloudLiveStreamUrl", "" } };
        auto comparisonBlock =
            generateCompareValuesBlock( itemId, "==", "cameraStream", "cameraStream", valueConstant );
        scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

        SceneID sceneId = createScene( scene, __LINE__ );

        // set to itself
        setItemValue( makeRemote(), itemId, valueConstant, __LINE__ );

        testRunProgressBroadcast( scene, sceneId, "started", __LINE__ );
        testRunProgressBroadcast( scene, sceneId, "finished", __LINE__ );

        deleteScene( sceneId, __LINE__ );
    }
    {  // valueType with operator ==, but without operator<
        Json scene = Tests::createEmptyScene;
        Json valueConstant = { { "localRtspUrl", "rtsp://127.0.0.1:554" }, { "cloudLiveStreamUrl", "" } };
        auto comparisonBlock =
            generateCompareValuesBlock( itemId, "<=", "cameraStream", "cameraStream", valueConstant );
        scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

        auto answer = nma().sendtoHubAndGetAnswer( scene );
        LOGGER_INFO( cc::j( answer ) );
        Json error;
        EXPECT_TRUE( json_get( answer, "error", error ) );
        EXPECT_EQ( json_get_int( error, "code", -1 ), -32500 );
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.wrong" );
    }
}

TEST_F( CompareValuesTests, fixedValueTypeDiffer ) /* 3.1 */ {
    using namespace CoreD::Model::Resources;
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::CameraStream, __LINE__ );
    std::string itemId = device.m_itemId;

    {  // valueType of a when block differs from a value type of constant
        Json scene = Tests::createEmptyScene;
        Json valueConstant = { { "localRtspUrl", "rtsp://127.0.0.1:554" }, { "cloudLiveStreamUrl", "" } };
        auto comparisonBlock =
            generateCompareValuesBlock( itemId, "==", "cameraStream", "string", "some value constant" );
        scene[ "params" ][ "when" ] = JSON_ARRAY_( comparisonBlock );

        auto answer = nma().sendtoHubAndGetAnswer( scene );
        LOGGER_INFO( cc::j( answer ) );
        Json error;
        EXPECT_TRUE( json_get( answer, "error", error ) );
        EXPECT_EQ( json_get_int( error, "code", -1 ), -32500 );
        EXPECT_EQ( json_get_str( error, "data" ), "scenes.when.type.mismatch" );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
