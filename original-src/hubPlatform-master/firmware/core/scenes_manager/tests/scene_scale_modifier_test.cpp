#include "scenes_manager/modify_scene/modify_scene.h"
#include "scenes_manager/modify_scene/modify_scene_on_fetch.h"
#include "scenes_manager/modify_scene/modify_scene_on_create.h"
#include "scenes_manager/modify_scene/resources.h"
#include "scenes_manager/modify_scene/items_transport.h"
#include "scenes_manager/tests/resources/scene_scale_resources.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

/*

TEST PLAN

DONE    0. Run composite test including OR operator, setItemValue, compareNumbers, isItemState, all in one
DONE    1. Test single when-block methods:
DONE        1.1 isItemState
DONE        1.2 compareNumbers
DONE        1.3 compareNumberRange
DONE    2. Test operator blocks
DONE        2.1 AND operator
DONE        2.2 OR operator
DONE        2.3 NOT operator
DONE        2.4 XOR operator
DONE        2.5 function operator

*/

namespace HubPlatform {
namespace UT {

class FakeItemsTransport : public ScenesManager::ItemsTransport {
public:
    std::vector< Json > getItemsBy( const JsonObject& condition ) override {
        static std::vector< Json > thermostatSetPoints = { SceneScale::itemThermostatSetPoint,
                                                           SceneScale::itemSetPointHeating };

        LOGGER_TRACE_RAW( "FakeItemsTransport::getItemsBy: Ignoring condition for now: " + cc::j( condition ) );
        return thermostatSetPoints;
    }
};

using ::testing::Test;

class SceneScaleModifierTests : public Test {
public:
    static void SetUpTestCase();

    static Json getItem( const Json& scene, const JsonPath& keyPath );
    static void testStrategyOnFetch( const Json& scene,
                                     const std::string& defaultScale,
                                     int codeLine,
                                     Json* modifiedScene = nullptr );
    static void testStrategyOnCreate( const Json& scene,
                                      const std::string& defaultScale,
                                      int codeLine,
                                      Json* modifiedScene = nullptr );

    void testStrategies( const Json& scene, int expectedSubstitutions, int codeLine );
    static size_t countSubstrings( const std::string& source, const std::string& pattern );
    static std::string getId( const Json& itemObject );
    static std::string getThermostatId();

protected:
    std::string m_defaultScale{ "celsius" };
    std::string m_defaultScaleOnFetch{ "fahrenheit" };
};

void SceneScaleModifierTests::SetUpTestCase() {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

Json SceneScaleModifierTests::getItem( const Json& scene, const JsonPath& keyPath ) {
    const auto& keys = keyPath.keys();
    const Json* current = &scene;

    try {
        for( const auto& key : keys ) {
            if( key.is_string() ) {
                const Json& found = current->at( key.get< std::string >() );
                current = &found;
            }
            else if( key.is_number() ) {
                const Json& found = current->at( key.get< int >() );
                current = &found;
            }
        }

        if( current )
            return *current;
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Exception: " ) + ex.what() );
    }
    return Json{};
}

void SceneScaleModifierTests::testStrategyOnFetch( const Json& scene,
                                                   const std::string& defaultScale,
                                                   int codeLine,
                                                   Json* modifiedScene ) {
    ScenesManager::ModifyScene sceneScaleModifier( ScenesManager::factoryModifySceneOnFetch() );
    auto modifiedSceneDump = sceneScaleModifier.processScene( scene, defaultScale );

    ASSERT_TRUE( modifiedSceneDump.first ) << "Should be Ok, at: " << __FILE__ << ":" << codeLine;
    LOGGER_TRACE( "modifiedSceneDump: " + cc::j( modifiedSceneDump.second ) );
    if( modifiedScene )
        *modifiedScene = std::move( modifiedSceneDump.second );
}

void SceneScaleModifierTests::testStrategyOnCreate( const Json& scene,
                                                    const std::string& defaultScale,
                                                    int codeLine,
                                                    Json* modifiedScene ) {
    FakeItemsTransport transport;
    ScenesManager::ModifyScene sceneScaleModifier( ScenesManager::factoryModifySceneOnCreate( transport ) );
    auto modifiedSceneDump = sceneScaleModifier.processScene( scene, defaultScale );

    ASSERT_TRUE( modifiedSceneDump.first ) << "Should be Ok, at: " << __FILE__ << ":" << codeLine;
    LOGGER_TRACE( "modifiedSceneDump: " + cc::j( modifiedSceneDump.second ) );
    if( modifiedScene )
        *modifiedScene = std::move( modifiedSceneDump.second );
}

void SceneScaleModifierTests::testStrategies( const Json& scene, int expectedSubstitutions, int codeLine ) {
    Json sceneWithScales = JSON_EMPTY_OBJ;
    testStrategyOnCreate( scene, m_defaultScale, codeLine, &sceneWithScales );
    EXPECT_EQ( countSubstrings( sceneWithScales.dump(), m_defaultScale ), expectedSubstitutions )
        << "Number of temperature scale occurrences";

    Json sceneWithConvertedScales = JSON_EMPTY_OBJ;
    testStrategyOnFetch( sceneWithScales, m_defaultScaleOnFetch, codeLine, &sceneWithConvertedScales );
    EXPECT_EQ( countSubstrings( sceneWithConvertedScales.dump(), m_defaultScaleOnFetch ), expectedSubstitutions )
        << "Number of temperature scale occurrences";
}

size_t SceneScaleModifierTests::countSubstrings( const std::string& source, const std::string& pattern ) {
    if( source.empty() || pattern.empty() )
        return 0;
    size_t counter = 0;
    size_t pos = 0;
    do {
        pos = source.find( pattern, pos );
        if( pos != std::string::npos ) {
            counter++;
            pos++;
        }
    } while( pos != std::string::npos );
    return counter;
}

std::string SceneScaleModifierTests::getId( const Json& itemObject ) {
    return json_get_str( itemObject, "_id" );
}

std::string SceneScaleModifierTests::getThermostatId() {
    return getId( SceneScale::itemSetPointHeating );
}

class Compose {
    using Keys = Tests::KeyStringList;

public:
    static Json numberRangeDefault() {
        return compareNumberRange(
            SceneScaleModifierTests::getThermostatId(), "between", "float", "0", "float", "100" );
    }
    static Json compareNumbersDefault( std::string value = "100" ) {
        return compareNumbers( SceneScaleModifierTests::getThermostatId(), "<", "float", value );
    }
    static Json isItemStateDefault( std::string value = "100" ) {
        std::string thermostatItemId = SceneScaleModifierTests::getThermostatId();
        Keys keys = {
            { "__ITEM_ID__", thermostatItemId }, { "__VALUE_TYPE__", "float" }, { "__COMPOUND_VALUE__", value }
        };
        return Tests::replaceTokens( SceneScale::isItemStateBlock, keys );
    }

    static Json compareNumberRange( std::string itemId,
                                    std::string comparator,
                                    std::string startType,
                                    std::string startValue,
                                    std::string endType,
                                    std::string endValue ) {
        Keys keys = { { "__ITEM_ID__", itemId },
                      { "__COMPARATOR__", comparator },
                      { "__START_VALUE_TYPE__", startType },
                      { "__START_COMPOUND_VALUE__", startValue },
                      { "__END_VALUE_TYPE__", endType },
                      { "__END_COMPOUND_VALUE__", endValue } };
        return Tests::replaceTokens( SceneScale::compareNumberRangeBlock, keys );
    }

    static Json compareNumbers( std::string itemId, std::string comparator, std::string type, std::string value ) {
        Tests::KeyStringList keys = { { "__ITEM_ID__", itemId },
                                      { "__COMPARATOR__", comparator },
                                      { "__VALUE_TYPE__", type },
                                      { "__COMPOUND_VALUE__", value } };
        return Tests::replaceTokens( SceneScale::compareNumbersBlock, keys );
    }

    static Json operatorBlock( std::string operatorName, const Json& whenBlockArray ) {
        Keys keys = { { "__OPERATOR_NAME__", operatorName }, { "__WHEN_BLOCK_ARRAY__", whenBlockArray.dump() } };
        return Tests::replaceTokens( SceneScale::anyOperatorBlock, keys );
    }

    static Json scene( std::string name, const JsonArray& whenArray, const JsonArray& thenArray = JSON_EMPTY_ARR ) {
        Keys keys = { { "__SCENE_NAME__", name },
                      { "__WHEN_BLOCK_ARRAY__", whenArray.dump() },
                      { "__THEN_BLOCK_ARRAY__", thenArray.dump() } };
        return Tests::replaceTokens( SceneScale::emptyScene, keys );
    }
};

TEST_F( SceneScaleModifierTests, run ) /* 0 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    using namespace SceneEngine;

    std::vector< SceneScaleModifier::ItemFound > itemsFound;

    SceneScaleModifier modifier( SceneScale::testScene, m_defaultScale );
    modifier.onItemFound(
        [&itemsFound]( const SceneScaleModifier::ItemFound& item ) { itemsFound.push_back( item ); } );

    ASSERT_TRUE( modifier.process() ) << "Scene processing has failed";
    EXPECT_EQ( modifier.modificationCount(), 2 );
    EXPECT_EQ( itemsFound.size(), 2 ) << "2 items with numeric type (int or float) should be found";

    for( const auto& item : itemsFound ) {
        LOGGER_INFO( "ItemId found: " + item.key.getSourceId() + ", path: " + item.path.toString() );
        LOGGER_INFO( "Value block: " + item.valueBlock.dump() );

        Json extracted = getItem( modifier.getScene(), item.path );
        LOGGER_DEBUG( cc::debug( "Extracted by path: " ) + extracted.dump() );

        EXPECT_FALSE( item.valueBlockRelativePath.empty() );

        Json tmp = item.valueBlock;
        tmp[ "scale" ] = "celsius";
        tmp[ "value" ] = tmp[ "value" ].get< int >() + 5;

        JsonPath pathToUpdate( item.path );
        pathToUpdate.append( item.valueBlockRelativePath );
        EXPECT_TRUE( modifier.updateItemValue( pathToUpdate, tmp ) ) << "Update operation";
    }

    auto sceneDump = modifier.getScene().dump();
    LOGGER_DEBUG_RAW( "Processed scene: " + sceneDump );
    ASSERT_TRUE( sceneDump.find( "\"celsius\"" ) != std::string::npos )
        << "Should have substring \"celsius\" in it if core items found and they are temperature";

    const auto defaultScale = "fahrenheit";
    testStrategyOnFetch( modifier.getScene(), defaultScale, __LINE__, nullptr );
}

TEST_F( SceneScaleModifierTests, strategyTest_isItemState ) /* 1.1 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    const int expectedSubstitutions = 1;
    Json whenBlock = Compose::isItemStateDefault();
    Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( whenBlock ) );
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_compareNumbers ) /* 1.2 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    Json whenBlock = Compose::compareNumbersDefault();
    Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( whenBlock ) );
    const int expectedSubstitutions = 1;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_compareNumberRange ) /* 1.3 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( whenBlock ) );
    const int expectedSubstitutions = 2;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_AND ) /* 2.1 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    auto whenBlock2 = Compose::compareNumbersDefault();
    Json operatorBlock =
        Compose::operatorBlock( SceneEngine::MethodNames::AndOperator, JSON_ARRAY_( whenBlock, whenBlock2 ) );
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( operatorBlock ) );
    const int expectedSubstitutions = 3;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_OR ) /* 2.2 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    auto whenBlock2 = Compose::compareNumbersDefault();
    Json operatorBlock =
        Compose::operatorBlock( SceneEngine::MethodNames::OrOperator, JSON_ARRAY_( whenBlock, whenBlock2 ) );
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( operatorBlock ) );
    const int expectedSubstitutions = 3;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_NOT ) /* 2.3 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    Json operatorBlock = Compose::operatorBlock( SceneEngine::MethodNames::NotOperator, JSON_ARRAY_( whenBlock ) );
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( operatorBlock ) );
    const int expectedSubstitutions = 2;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_XOR ) /* 2.4 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    auto whenBlock2 = Compose::compareNumbersDefault();
    Json operatorBlock =
        Compose::operatorBlock( SceneEngine::MethodNames::XorOperator, JSON_ARRAY_( whenBlock, whenBlock2 ) );
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( operatorBlock ) );
    const int expectedSubstitutions = 3;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

TEST_F( SceneScaleModifierTests, strategyTest_function ) /* 2.5 */ {
    LOGGER_TRACE_RAW( std::string{ "# Test name: " } + test_info_->name() );
    auto whenBlock = Compose::numberRangeDefault();
    auto whenBlock2 = Compose::compareNumbersDefault();
    Json operatorBlock =
        Compose::operatorBlock( SceneEngine::MethodNames::FunctionOperator, JSON_ARRAY_( whenBlock, whenBlock2 ) );
    const Json scene = Compose::scene( test_info_->name(), JSON_ARRAY_( operatorBlock ) );
    const int expectedSubstitutions = 3;
    testStrategies( scene, expectedSubstitutions, __LINE__ );
}

}  // namespace UT
}  // namespace HubPlatform
