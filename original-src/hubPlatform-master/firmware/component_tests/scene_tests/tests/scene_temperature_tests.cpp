#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"

#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"

#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "cored/model/resources/cored_resources.h"

#include "jsonrpc/jsonrpc.h"
#include "database_client/dbtools.h"
#include "database_client/dbqueryparser.h"

#include "base_utils/gregorian_calendar.h"

#include <gtest/gtest.h>
#include <functional>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN
    DONE    1. setItemValue command for temperature/scalable, then block
    DONE    2. compareNumbers({value: N, scale: <scale>}) for scalable
    DONE    3. isItemState for scalable

*/

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

const std::string celsius{ CoreD::Model::ValueScales::Celsius };
const std::string fahrenheit{ CoreD::Model::ValueScales::Fahrenheit };

class SceneTemperatureTests : public SceneManagerTestsFixture, public WithParamInterface< std::string > {
public:
    void SetUp() override;
    void TearDown() override;
};

void SceneTemperatureTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneTemperatureTests::TearDown() {
    clearExistingDevices( __LINE__ );
    SceneManagerTestsFixture::TearDown();
}

std::string printer( const TestParamInfo< SceneTemperatureTests::ParamType >& info ) {
    return info.param;
}

INSTANTIATE_TEST_CASE_P( DifferentHubTemperatureScale, SceneTemperatureTests, Values( celsius, fahrenheit ), printer );

TEST_P( SceneTemperatureTests, setItemValueWithScale ) /* 1 */ {
    std::string testScale = GetParam();
    auto& coreRemote = makeRemote( nma() );
    setDefaultTemperatureScaleSetting( testScale, __LINE__ );

    auto deviceId =
        addDevice( coreRemote, "Z-Wave Thermostat", "thermostat", "139_21586_21570", "hvac", "", false, __LINE__ );

    using namespace HubPlatform::CoreD;
    const std::string scale = Model::ValueScales::Celsius;
    double valueInScale = 18;
    const double sceneSetValueInCelsius = 22;
    auto value = Model::ValueFactory::createItemValue( Model::ValueType::Enum::Temperature,
                                                       JSON( "value" << valueInScale << "scale" << scale ) );

    Model::Item setPointItem(
        deviceId, "thermostat_setpoint", Model::ValueType::Enum::Temperature, true, true, std::move( value ) );

    auto setPointAddResult = coreRemote.addItem( setPointItem );
    Json broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );

    const auto& itemId = setPointAddResult.first;
    EXPECT_TRUE( setPointAddResult.second ) << "Item was not created";

    // create scene with setItemValue action
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    auto response = scenesManagerInterface.setTestClock( std::chrono::seconds( 1562505419LL ) );
    ASSERT_TRUE( response[ "error" ].empty() );

    auto createSetItemValueAction =
        replaceTokens( HubPlatform::Tests::createSetItemValueAction,
                       { { "__ITEM_ID__", itemId },
                         { "12345", std::to_string( sceneSetValueInCelsius ) + R"(, "scale": "celsius")" },
                         { "__ITEM_VALUE_TYPE__", "float" } } );

    std::string sceneId = createScene( createSetItemValueAction );

    // check scene execution
    this->runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( createSetItemValueAction, sceneId, "started", __LINE__ );

    using namespace std::placeholders;
    using Sig1 = void ( SceneManagerTestsFixture::* )(
        const JsonObject&, const JsonObject&, const std::string&, const std::string&, int, const JsonArray& );
    JsonArray arr = JSON_EMPTY_ARR;
    auto testRunProgressBroadcastBind = std::bind(
        static_cast< Sig1 >( &SceneManagerTestsFixture::testRunProgressBroadcast ), this, _1, _2, _3, _4, _5, arr );

    using Sig2 = void ( SceneManagerTestsFixture::* )(
        const JsonObject&, CoreDRemoteInterface::CoreDInterface::ItemID, double, int );
    auto checkItemUpdateBroadcastBind =
        std::bind( static_cast< Sig2 >( &SceneManagerTestsFixture::checkItemUpdateBroadcast ), this, _1, _2, _3, _4 );

    checkBroadcasts(
        { { "hub.scene.run.progress",
            std::bind( testRunProgressBroadcastBind, _1, createSetItemValueAction, sceneId, "finished", __LINE__ ) },
          { "hub.item.updated",
            std::bind( checkItemUpdateBroadcastBind,
                       _1,
                       itemId,
                       convertToScale( sceneSetValueInCelsius, celsius.c_str(), testScale.c_str() ),
                       __LINE__ ) } },
        __LINE__ );

    auto temperatureType = CoreD::Model::ValueType( Model::ValueType::Enum::Temperature );
    auto temperatureConverter = temperatureType.getConverter();

    auto itemValue = coreRemote.getItem( itemId );
    auto dump = itemValue.dump();
    // value must be {value: 22, scale: celsius}
    LOGGER_TRACE( "itemValue: " + cc::j( itemValue ) );
    Json jScalable;
    EXPECT_TRUE( json_get_obj( itemValue, "value", jScalable ) );
    EXPECT_FLOAT_EQ(
        json_get_float( jScalable, "value" ),
        temperatureConverter(
            sceneSetValueInCelsius, Model::ValueScales::Celsius, json_get_str( jScalable, "scale" ).c_str() ) );
}

TEST_P( SceneTemperatureTests, compareNumbersScaleable ) /* 2 */ {
    using namespace CoreD::Model::Resources;
    std::string testScale = GetParam();
    auto& coreRemote = makeRemote( nma() );

    setDefaultTemperatureScaleSetting( testScale, __LINE__ );
    auto deviceId =
        addDevice( coreRemote, "testDevice1", "thermostat", "139_21586_21570", "hvac", "", false, __LINE__ );

    using namespace HubPlatform::CoreD;
    const std::string scale = Model::ValueScales::Celsius;
    double valueInScale = 18;                  // 64.4 F
    const double sceneSetValueInCelsius = 22;  // 71.6 F
    auto value = Model::ValueFactory::createItemValue( Model::ValueType::Enum::Temperature,
                                                       JSON( "value" << valueInScale << "scale" << scale ) );

    Model::Item setPointItem(
        deviceId, "thermostat_setpoint", Model::ValueType::Enum::Temperature, true, true, std::move( value ) );

    auto setPointAddResult = coreRemote.addItem( setPointItem );
    auto itemId1 = setPointAddResult.first;

    Json broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
    double itemDefaultValue = 2.0f;  // 35.6 F
    LOGGER_TRACE_RAW( "Item default value, celsius: " + cc::num( itemDefaultValue ) +
                      ", fahrenheit: " + cc::num( toFahrenheit( itemDefaultValue ) ) );

    coreRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << itemDefaultValue
                                                           << "scale" << scale ) );
    checkItemUpdateBroadcast(
        itemId1, convertToScale( itemDefaultValue, celsius.c_str(), testScale.c_str() ), __LINE__ );

    auto request = replaceTokens( createCompareNumbersScalableScene,
                                  { { "__ITEM_ID1__", itemId1 },
                                    { "__ITEM1_TYPE__", "float" },
                                    { "11112345", std::to_string( sceneSetValueInCelsius ) },
                                    { "__ITEM1_SCALE__", celsius } } );
    SceneID sceneId = createScene( request );

    // 25 C == 77 F, the scene should raise after 22 C
    LOGGER_TRACE_RAW( "Raise the scene, celsius: " + cc::num( sceneSetValueInCelsius + 3 ) +
                      ", fahrenheit: " + cc::num( toFahrenheit( sceneSetValueInCelsius + 3 ) ) );

    coreRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName
                                                           << ( sceneSetValueInCelsius + 3 ) << "scale" << scale ) );
    checkItemUpdateBroadcast(
        itemId1, convertToScale( sceneSetValueInCelsius + 3, celsius.c_str(), testScale.c_str() ), __LINE__ );

    auto checkLastBroadcast = [&](
                                  CoreDRemoteInterface::CoreDInterface::ItemID itemId, float itemValue, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject broadcast = waitForBroadcast( "hub.scene.run.progress", getCodeLineString( codeLine ) );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
            JsonObject result = JSON_EMPTY_OBJ;
            EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
            if( json_get_str( result, "status" ) == "started" ) {
                testRunProgressBroadcast( broadcast, createCompareNumbersScalableScene, sceneId, "started", codeLine );
            }
            else {
                testRunProgressBroadcast( broadcast, createCompareNumbersScalableScene, sceneId, "finished", codeLine );
            }
        }
        else {
            checkItemUpdateBroadcast( broadcast, itemId, itemValue, codeLine );
        }
    };
    testRunProgressBroadcast( createCompareNumbersScalableScene, sceneId, "started", __LINE__ );
    checkLastBroadcast(
        itemId1, convertToScale( sceneSetValueInCelsius + 3, celsius.c_str(), testScale.c_str() ), __LINE__ );

    deleteScenes();
}

TEST_P( SceneTemperatureTests, isItemStateScaleable ) /* 3 */ {
    using namespace CoreD::Model::Resources;
    std::string testScale = GetParam();
    auto& coreRemote = makeRemote( nma() );

    setDefaultTemperatureScaleSetting( testScale, __LINE__ );
    auto deviceId =
        addDevice( coreRemote, "testDevice1", "thermostat", "139_21586_21570", "hvac", "", false, __LINE__ );

    using namespace HubPlatform::CoreD;
    const std::string scale = Model::ValueScales::Celsius;
    double valueInScale = 18;                  // 64.4 F
    const double sceneSetValueInCelsius = 22;  // 71.6 F
    auto value = Model::ValueFactory::createItemValue( Model::ValueType::Enum::Temperature,
                                                       JSON( "value" << valueInScale << "scale" << scale ) );

    Model::Item setPointItem(
        deviceId, "thermostat_setpoint", Model::ValueType::Enum::Temperature, true, true, std::move( value ) );

    auto setPointAddResult = coreRemote.addItem( setPointItem );
    auto itemId1 = setPointAddResult.first;

    Json broadcast = waitForBroadcast( "hub.item.added", NMA_TEST_MAKE_MESSAGE( "" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
    double itemDefaultValue = 2.0f;  // 35.6 F
    LOGGER_TRACE_RAW( "Item default value, celsius: " + cc::num( itemDefaultValue ) +
                      ", fahrenheit: " + cc::num( toFahrenheit( itemDefaultValue ) ) );

    coreRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << itemDefaultValue
                                                           << "scale" << scale ) );
    checkItemUpdateBroadcast(
        itemId1, convertToScale( itemDefaultValue, celsius.c_str(), testScale.c_str() ), __LINE__ );

    auto isItemStateBlock = replaceTokensInString(
        isItemStateWhenBlock,
        { { "__ITEM_ID__", itemId1 },
          { "__TYPE__", "float" },
          { "__VALUE__", std::to_string( sceneSetValueInCelsius ) + R"(, "scale": "celsius")" } } );
    auto sceneRequest = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateEnabling" }, { "__WHEN_BLOCKS__", isItemStateBlock }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( sceneRequest );

    // 25 C == 77 F, the scene should raise after 22 C
    LOGGER_TRACE_RAW( "Raise the scene, celsius: " + cc::num( sceneSetValueInCelsius ) +
                      ", fahrenheit: " + cc::num( toFahrenheit( sceneSetValueInCelsius ) ) );

    coreRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName
                                                           << sceneSetValueInCelsius << "scale" << celsius ) );
    checkItemUpdateBroadcast(
        itemId1, convertToScale( sceneSetValueInCelsius, celsius.c_str(), testScale.c_str() ), __LINE__ );

    auto checkLastBroadcast = [&](
                                  CoreDRemoteInterface::CoreDInterface::ItemID itemId, float itemValue, int codeLine ) {
        auto codeLineStr = getCodeLineString( codeLine );
        JsonObject broadcast = waitForBroadcast( "hub.scene.run.progress", getCodeLineString( codeLine ) );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.scene.run.progress" ) {
            JsonObject result = JSON_EMPTY_OBJ;
            EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
            if( json_get_str( result, "status" ) == "started" ) {
                testRunProgressBroadcast( broadcast, sceneRequest, sceneId, "started", codeLine );
            }
            else {
                testRunProgressBroadcast( broadcast, sceneRequest, sceneId, "finished", codeLine );
            }
        }
        else {
            checkItemUpdateBroadcast( broadcast, itemId, itemValue, codeLine );
        }
    };
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );
    checkLastBroadcast(
        itemId1, convertToScale( sceneSetValueInCelsius + 3, celsius.c_str(), testScale.c_str() ), __LINE__ );

    deleteScenes();
}

}  // namespace Tests
}  // namespace HubPlatform
