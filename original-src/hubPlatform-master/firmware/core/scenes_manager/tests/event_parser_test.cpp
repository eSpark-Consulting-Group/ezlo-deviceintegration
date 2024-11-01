#include "scenes_manager/scenes_processor/parsers/event_parser.h"
#include "scenes_manager/scenes_processor/parsers/event_parser_interface.h"
#include "service_interfaces/task_loop.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include "common_utils/cpp14.h"

#include "base_utils/utils.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"
#include "scenes_manager/scenes_engine/helpers/test_clock.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN

DONE    1. isIntersection
DONE    2. createRange
DONE    3. convertComparator
DONE    4. checkEvents
DONE    5. wrong when block
DONE        5.1 Error: scene.when is not an array
DONE        5.2 Error: scene.exceptions is not an array
DONE    6. expressions
DONE        6.1 item event expression
DONE        6.2 compareNumbers event expression
DONE    7. DateTime
DONE        7.1 weeks
DONE        7.2 yearWeeks
DONE        7.3 weekdays
DONE        7.4 days of month
DONE    8. compareNumberRange
DONE    9. checkSoftwareUpdate
DONE    10 Function operator
DONE        10.1 function For(Sustained For)
DONE            10.1.1 function.for method=less
DONE            10.1.2 function.for method=least (aka FOR function)
DONE            10.1.3 function.for errors
DONE        10.2 function.repeat
DONE        10.3 function Latch
DONE    12 isDictionaryChanged
DONE    13 compareStrings
DONE    14 stringOperation
DONE    15 hotzone events
DONE        15.1 any hotzone for any object
DONE        15.2 any hotzone for "any" filter
DONE        15.3 any hotzone for "anyOf" filter
DONE        15.4 specific hotzones for "any" filter
DONE        15.6 filters should not be provided
DONE        15.7 filters are missing
DONE    16 inArray
DONE        16.1 inArray with correct data
DONE        16.2 inArray with value which is not an array
DONE        16.3 inArray when value doesn't match type
DONE    17 compareValues
DONE        17.1 compareValues: compare values with isLess (operator <)
DONE        17.2 compareValues: compare values without isLess (operator <)
DONE    18 function.follow
DONE    19 function.pulse
DONE        19.1 function.pulse only required parameters
DONE        19.2 function.pulse all parameters
DONE    20 isItemStateChanged
DONE        20.1 isItemStateChanged "any" -> "any" valid case
DONE        20.2 isItemStateChanged "value" -> "any" valid case
DONE        20.3 isItemStateChanged "value" -> "value" valid case
DONE        20.4 isItemStateChanged with invalid CoreD values for "start" and "finish" params
DONE    21 exceptions
DONE        21.1 no errors without the block
DONE        21.2 events parsed correctly
DONE        21.3 exceptions parsed correctly alongside with triggers
DONE        21.4 exceptions: add the same placeholder to all events
DONE        21.5 exceptions: add the same placeholder to an operator and event
DONE    22 isSunState events
DONE        22.1 isSunState range event
DONE        22.2 isSunState time point event
DONE    23 EventParser ctor exceptions

 */

using KeyStringList = std::vector< std::pair< std::string, std::string > >;
std::string replaceTokens( std::string source, const KeyStringList& keys ) {
    for( const auto& it : keys ) {
        replaceAll( source, it.first, it.second );
    }
    return source;
}

class EventParserTest : public testing::Test {
public:
    void SetUp() override;
    bool isIntersection( std::multimap< std::string, Range< Json > >& compareNumbersRanges,
                         const std::string& itemId,
                         const JsonObject& itemValue,
                         const std::string& comparator );
    Range< Json > createRange( const JsonObject& json, const std::string& comparator );
    SceneEngine::ItemValueComparator convertComparator( const std::string& comparator, bool hasIsLess = true );
    SceneEngine::EventContainer parseTriggers( Json& sceneJsonObject );
    SceneEngine::EventContainer parseExceptions( Json& sceneJsonObject );

    void testFunctionOperatorSuccessCase( const std::string& function );

    static JsonObject createInArrayWhenBlock( const std::string& type, const std::string& value );
    static JsonObject createIsItemStateChangedWhenBlock( const std::string& startValue,
                                                         const std::string& finishValue );

    void shouldThrow( Json& sceneJson, int line );
    void shouldBeOk( Json& sceneJson, int line );
    void testLatch( Json sceneJson,
                    std::string whenBlock,
                    const char* settings,
                    std::function< void( Json&, int ) > checker,
                    int line );

protected:
    std::unique_ptr< ScenesManager::EventParser > m_eventParser;
    ScenesManager::IdUtils::PlaceholdersMapping m_placeholdersMapping;

private:
    HubPlatform::Services::TaskLoop m_taskLoop;
    std::unique_ptr< SceneEngine::Clock > m_clock;
};

namespace {

class FakeGenericItemValueProvider : public SceneEngine::ItemValueProviderInterface {
public:
    FakeGenericItemValueProvider( SceneEngine::ValueSourceKey key )
    : m_key( std::move( key ) ) {}
    bool fetchItemValue() override {
        return false;
    };
    const SceneEngine::ItemValueWrapper& getCachedItemValue() const override {
        static SceneEngine::ItemValueWrapper wrapper;
        return wrapper;
    };
    const Json& getCachedError() const override {
        static Json json;
        return json;
    };

    bool hasValueKey() const override {
        return true;
    };
    const SceneEngine::ValueSourceKey& getValueKey() const override {
        return m_key;
    };

private:
    SceneEngine::ValueSourceKey m_key;
};

}  // namespace

void EventParserTest::SetUp() {
    m_clock = std::make_unique< SceneEngine::Clock >();
    m_eventParser = std::make_unique< ScenesManager::EventParser >(
        "",
        []() { return HubPlatform::CoreDRemoteInterface::GeoPosition(); },
        [this]() -> SceneEngine::IClock& { return *m_clock; },
        []( const SceneEngine::ValueSourceKey& key ) -> SceneEngine::ItemValueProviderPtr {
            return std::make_unique< FakeGenericItemValueProvider >( key );
        },
        m_taskLoop );
}

bool EventParserTest::isIntersection( std::multimap< std::string, Range< Json > >& compareNumbersRanges,
                                      const std::string& itemId,
                                      const JsonObject& itemValue,
                                      const std::string& comparator ) {
    return m_eventParser->isIntersection( compareNumbersRanges, itemId, itemValue, comparator );
}

Range< Json > EventParserTest::createRange( const JsonObject& json, const std::string& comparator ) {
    return m_eventParser->createRange( json, comparator );
}

SceneEngine::ItemValueComparator EventParserTest::convertComparator( const std::string& comparator, bool hasIsLess ) {
    return m_eventParser->convertComparator( comparator, hasIsLess );
}

SceneEngine::EventContainer EventParserTest::parseTriggers( Json& sceneJsonObject ) {
    return m_eventParser->parseTriggers( sceneJsonObject, m_placeholdersMapping );
}

SceneEngine::EventContainer EventParserTest::parseExceptions( Json& sceneJsonObject ) {
    return m_eventParser->parseExceptions( sceneJsonObject, m_placeholdersMapping );
}

void EventParserTest::testFunctionOperatorSuccessCase( const std::string& function ) {
    std::string whenBlockStr = SceneEngine::UT::compareNumbersRangeCondition.dump();
    std::string functionOperatorStr =
        replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                       { { "__BLOCKS__", std::move( whenBlockStr ) }, { "__FUNCTION__", function } } );

    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { Json::parse( functionOperatorStr ) };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getCompareNumbersRangeEvents();
    EXPECT_EQ( collection.size(), 1 );
}

JsonObject EventParserTest::createInArrayWhenBlock( const std::string& type, const std::string& value ) {
    KeyStringList substitute = { { "__METHOD__", "inArray" },
                                 { "__ITEM_ID__", "aaa" },
                                 { "__OPERATION__", SceneEngine::Model::Resources::ArrayOperations::In },
                                 { "__TYPE__", type },
                                 { "__VALUE__", value } };
    const auto whenTemplate = replaceTokens( SceneEngine::UT::itemOperationWhenBlockTemplate, substitute );
    JsonObject whenBlock;
    EXPECT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );
    return whenBlock;
}

JsonObject EventParserTest::createIsItemStateChangedWhenBlock( const std::string& startValue,
                                                               const std::string& finishValue ) {
    KeyStringList substitute = {
        { "__ITEM_ID__", "aaa" }, { "__START_VALUE__", startValue }, { "__FINISH_VALUE__", finishValue }
    };
    auto whenTemplate = replaceTokens( SceneEngine::UT::isItemStateChangedWhenBlockTemplate, substitute );
    JsonObject whenBlock;
    EXPECT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );
    return whenBlock;
}

void EventParserTest::shouldThrow( Json& sceneJson, int line ) {
    try {
        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        FAIL() << "Should throw, but succeeded, at: " << __FILE__ << ":" << line;
    }
    catch( const std::exception& ex ) {
        Log::info( std::string{ "Throws as expected: " } + ex.what() );
    }
}

void EventParserTest::shouldBeOk( Json& sceneJson, int line ) {
    try {
        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        const auto& collection = events.getCompareNumbersRangeEvents();
        EXPECT_EQ( collection.size(), 1 ) << __FILE__ << ":" << line;
    }
    catch( const std::exception& ex ) {
        FAIL() << "Should not throw, but throws: " << ex.what() << ", at: " << __FILE__ << ":" << line;
    }
}

void EventParserTest::testLatch( Json sceneJson,
                                 std::string whenBlock,
                                 const char* settings,
                                 std::function< void( Json&, int ) > checker,
                                 int line ) {
    auto functionOperatorStr = replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                                              { { "__BLOCKS__", whenBlock }, { "__FUNCTION__", settings } } );

    Json functionOperator = Json::parse( functionOperatorStr );
    Log::info( "#  Function Latch: " + cc::j( functionOperator ) );
    sceneJson[ "when" ] = { functionOperator };

    ASSERT_TRUE( checker != nullptr ) << "checker should not be empty: " << __FILE__ << ":" << line;
    if( checker != nullptr ) {
        checker( sceneJson, line );
    }
}

TEST_F( EventParserTest, isIntersection ) /* 1 */ {
    auto itemId = "itemId1";
    Json itemValue( 5 );
    std::multimap< std::string, Range< Json > > compareNumbersRanges;
    auto testIntersection =
        [this, &compareNumbersRanges, &itemId, &itemValue](
            const std::string& comparator1, const std::string& comparator2, bool result, int codeLine ) {
            compareNumbersRanges.clear();
            EXPECT_TRUE( isIntersection( compareNumbersRanges, itemId, itemValue, comparator1 ) )
                << "code line: " << std::to_string( codeLine );
            EXPECT_EQ( isIntersection( compareNumbersRanges, itemId, itemValue, comparator2 ), result )
                << "code line: " << std::to_string( codeLine );
        };

    testIntersection( "==", "==", true, __LINE__ );
    testIntersection( "==", "!=", false, __LINE__ );
    testIntersection( "==", ">", false, __LINE__ );
    testIntersection( "==", "<", false, __LINE__ );
    testIntersection( "==", ">=", true, __LINE__ );
    testIntersection( "==", "<=", true, __LINE__ );

    testIntersection( "!=", "==", false, __LINE__ );
    testIntersection( "!=", "!=", true, __LINE__ );
    testIntersection( "!=", ">", true, __LINE__ );
    testIntersection( "!=", "<", true, __LINE__ );
    testIntersection( "!=", ">=", true, __LINE__ );
    testIntersection( "!=", "<=", true, __LINE__ );

    testIntersection( ">", "==", false, __LINE__ );
    testIntersection( ">", "!=", true, __LINE__ );
    testIntersection( ">", ">", true, __LINE__ );
    testIntersection( ">", "<", false, __LINE__ );
    testIntersection( ">", ">=", true, __LINE__ );
    testIntersection( ">", "<=", false, __LINE__ );

    testIntersection( ">=", "==", true, __LINE__ );
    testIntersection( ">=", "!=", true, __LINE__ );
    testIntersection( ">=", ">", true, __LINE__ );
    testIntersection( ">=", "<", false, __LINE__ );
    testIntersection( ">=", ">=", true, __LINE__ );
    testIntersection( ">=", "<=", true, __LINE__ );

    testIntersection( "<", "==", false, __LINE__ );
    testIntersection( "<", "!=", true, __LINE__ );
    testIntersection( "<", ">", false, __LINE__ );
    testIntersection( "<", "<", true, __LINE__ );
    testIntersection( "<", ">=", false, __LINE__ );
    testIntersection( "<", "<=", true, __LINE__ );

    testIntersection( "<=", "==", true, __LINE__ );
    testIntersection( "<=", "!=", true, __LINE__ );
    testIntersection( "<=", ">", false, __LINE__ );
    testIntersection( "<=", "<", true, __LINE__ );
    testIntersection( "<=", ">=", true, __LINE__ );
    testIntersection( "<=", "<=", true, __LINE__ );

    auto testIntersectionDifferentItemValues1 =
        [this, &compareNumbersRanges, &itemId](
            const std::string& comparator1, const std::string& comparator2, bool result, int codeLine ) {
            compareNumbersRanges.clear();
            EXPECT_TRUE( isIntersection( compareNumbersRanges, itemId, Json( 10 ), comparator1 ) )
                << "code line: " << std::to_string( codeLine );
            EXPECT_EQ( isIntersection( compareNumbersRanges, itemId, Json( 20 ), comparator2 ), result )
                << "code line: " << std::to_string( codeLine );
        };

    testIntersectionDifferentItemValues1( "==", "==", false, __LINE__ );
    testIntersectionDifferentItemValues1( "==", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( "==", ">", false, __LINE__ );
    testIntersectionDifferentItemValues1( "==", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( "==", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues1( "==", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues1( "!=", "==", true, __LINE__ );
    testIntersectionDifferentItemValues1( "!=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( "!=", ">", true, __LINE__ );
    testIntersectionDifferentItemValues1( "!=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( "!=", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues1( "!=", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues1( ">", "==", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">", ">", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues1( ">=", "==", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">=", ">", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">=", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues1( ">=", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues1( "<", "==", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( "<", ">", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( "<", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues1( "<=", "==", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues1( "<=", ">", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues1( "<=", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues1( "<=", "<=", true, __LINE__ );

    auto testIntersectionDifferentItemValues2 =
        [this, &compareNumbersRanges, &itemId](
            const std::string& comparator1, const std::string& comparator2, bool result, int codeLine ) {
            compareNumbersRanges.clear();
            EXPECT_TRUE( isIntersection( compareNumbersRanges, itemId, Json( 10 ), comparator1 ) )
                << "code line: " << std::to_string( codeLine );
            EXPECT_EQ( isIntersection( compareNumbersRanges, itemId, Json( 11 ), comparator2 ), result )
                << "code line: " << std::to_string( codeLine );
        };

    testIntersectionDifferentItemValues2( "==", "==", false, __LINE__ );
    testIntersectionDifferentItemValues2( "==", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( "==", ">", false, __LINE__ );
    testIntersectionDifferentItemValues2( "==", "<", true, __LINE__ );
    testIntersectionDifferentItemValues2( "==", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues2( "==", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues2( "!=", "==", true, __LINE__ );
    testIntersectionDifferentItemValues2( "!=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( "!=", ">", true, __LINE__ );
    testIntersectionDifferentItemValues2( "!=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues2( "!=", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues2( "!=", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues2( ">", "==", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">", ">", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">", "<", false, __LINE__ );
    testIntersectionDifferentItemValues2( ">", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues2( ">=", "==", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">=", ">", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">=", ">=", true, __LINE__ );
    testIntersectionDifferentItemValues2( ">=", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues2( "<", "==", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( "<", ">", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<", "<", true, __LINE__ );
    testIntersectionDifferentItemValues2( "<", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<", "<=", true, __LINE__ );

    testIntersectionDifferentItemValues2( "<=", "==", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<=", "!=", true, __LINE__ );
    testIntersectionDifferentItemValues2( "<=", ">", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<=", "<", true, __LINE__ );
    testIntersectionDifferentItemValues2( "<=", ">=", false, __LINE__ );
    testIntersectionDifferentItemValues2( "<=", "<=", true, __LINE__ );

    auto testIntersectionDifferentItemIds =
        [this, &compareNumbersRanges, &itemValue](
            const std::string& comparator1, const std::string& comparator2, bool result, int codeLine ) {
            compareNumbersRanges.clear();
            EXPECT_TRUE( isIntersection( compareNumbersRanges, "itemId1", itemValue, comparator1 ) )
                << "code line: " << std::to_string( codeLine );
            EXPECT_EQ( isIntersection( compareNumbersRanges, "itemId2", itemValue, comparator2 ), result )
                << "code line: " << std::to_string( codeLine );
        };

    testIntersectionDifferentItemIds( "==", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( "==", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( "==", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( "==", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( "==", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( "==", "<=", true, __LINE__ );

    testIntersectionDifferentItemIds( "!=", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( "!=", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( "!=", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( "!=", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( "!=", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( "!=", "<=", true, __LINE__ );

    testIntersectionDifferentItemIds( ">", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( ">", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( ">", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( ">", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( ">", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( ">", "<=", true, __LINE__ );

    testIntersectionDifferentItemIds( ">=", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( ">=", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( ">=", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( ">=", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( ">=", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( ">=", "<=", true, __LINE__ );

    testIntersectionDifferentItemIds( "<", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( "<", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( "<", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( "<", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( "<", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( "<", "<=", true, __LINE__ );

    testIntersectionDifferentItemIds( "<=", "==", true, __LINE__ );
    testIntersectionDifferentItemIds( "<=", "!=", true, __LINE__ );
    testIntersectionDifferentItemIds( "<=", ">", true, __LINE__ );
    testIntersectionDifferentItemIds( "<=", "<", true, __LINE__ );
    testIntersectionDifferentItemIds( "<=", ">=", true, __LINE__ );
    testIntersectionDifferentItemIds( "<=", "<=", true, __LINE__ );
}

TEST_F( EventParserTest, createRange ) /* 2 */ {
    const auto integerMax = Json( std::numeric_limits< JsonObject::number_integer_t >::max() );
    const auto integerMin = Json( std::numeric_limits< JsonObject::number_integer_t >::min() );
    const auto floatMax = Json( std::numeric_limits< JsonObject::number_float_t >::max() );
    const auto floatMin = Json( std::numeric_limits< JsonObject::number_float_t >::min() );
    const auto epsilon = std::numeric_limits< JsonObject::number_float_t >::epsilon();
    Json value( 5 );
    EXPECT_EQ( createRange( value, "==" ), Range< Json >( { value, value, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "!=" ), Range< Json >( { value, value, RangeType::Excluded } ) );
    EXPECT_EQ( createRange( value, ">" ), Range< Json >( { Json( 6 ), integerMax, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, ">=" ), Range< Json >( { Json( 5 ), integerMax, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "<" ), Range< Json >( { integerMin, Json( 4 ), RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "<=" ), Range< Json >( { integerMin, Json( 5 ), RangeType::Included } ) );
    value = 5.5f;
    EXPECT_EQ( createRange( value, "==" ), Range< Json >( { value, value, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "!=" ), Range< Json >( { value, value, RangeType::Excluded } ) );
    EXPECT_EQ( createRange( value, ">" ), Range< Json >( { Json( 5.5f + epsilon ), floatMax, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, ">=" ), Range< Json >( { Json( 5.5f ), floatMax, RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "<" ), Range< Json >( { floatMin, Json( 5.5f - epsilon ), RangeType::Included } ) );
    EXPECT_EQ( createRange( value, "<=" ), Range< Json >( { floatMin, Json( 5.5f ), RangeType::Included } ) );

    EXPECT_ANY_THROW( createRange( Json( "string" ), ">" ) );
    EXPECT_ANY_THROW( createRange( Json( true ), ">" ) );
    EXPECT_ANY_THROW( createRange( Json( nullptr ), ">" ) );
}

TEST_F( EventParserTest, convertComparator ) /* 3 */ {
    EXPECT_THROW( convertComparator( "equal" ), SceneEngine::SceneException );

    auto comparator = convertComparator( "==" );
    EXPECT_FALSE( comparator( { 3 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 4 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 5 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 6 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 7 }, { 5 } ) );

    comparator = convertComparator( "!=" );
    EXPECT_TRUE( comparator( { 3 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 4 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 5 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 6 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 7 }, { 5 } ) );

    comparator = convertComparator( ">" );
    EXPECT_FALSE( comparator( { 3 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 4 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 5 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 6 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 7 }, { 5 } ) );

    comparator = convertComparator( ">=" );
    EXPECT_FALSE( comparator( { 3 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 4 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 5 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 6 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 7 }, { 5 } ) );

    comparator = convertComparator( "<" );
    EXPECT_TRUE( comparator( { 3 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 4 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 5 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 6 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 7 }, { 5 } ) );

    comparator = convertComparator( "<=" );
    EXPECT_TRUE( comparator( { 3 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 4 }, { 5 } ) );
    EXPECT_TRUE( comparator( { 5 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 6 }, { 5 } ) );
    EXPECT_FALSE( comparator( { 7 }, { 5 } ) );
}

TEST_F( EventParserTest, convertComparatorWithoutLess ) /* 3.1 */ {
    bool valueTypeWithoutIsLess = false;
    EXPECT_THROW( convertComparator( "<", valueTypeWithoutIsLess ), SceneEngine::SceneException );
    EXPECT_THROW( convertComparator( ">", valueTypeWithoutIsLess ), SceneEngine::SceneException );
    EXPECT_THROW( convertComparator( "<=", valueTypeWithoutIsLess ), SceneEngine::SceneException );
    EXPECT_THROW( convertComparator( ">=", valueTypeWithoutIsLess ), SceneEngine::SceneException );

    EXPECT_NO_THROW( convertComparator( "==", valueTypeWithoutIsLess ) );
    EXPECT_NO_THROW( convertComparator( "!=", valueTypeWithoutIsLess ) );
}

TEST_F( EventParserTest, checkEvents ) /* 4 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = SceneEngine::UT::whenBlockArray;
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    EXPECT_EQ( events.intervalEvents.size(), 1 );
    EXPECT_EQ( events.sunStateEvents.size(), 1 );
    EXPECT_EQ( events.dateEvents.size(), 2 );
    EXPECT_EQ( events.userLockEvents.size(), 0 );
    EXPECT_EQ( events.buttonStateEvents.size(), 0 );
    EXPECT_EQ( events.getCompareNumbersEvents().size(), 0 );
    EXPECT_EQ( events.getItemEvents().size(), 0 );
}

TEST_F( EventParserTest, wrongWhenBlock_notArray ) /* 5.1 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = SceneEngine::UT::wrongWhenBlockArray;
    try {
        parseTriggers( sceneJson );
        FAIL() << "Should be an exception";
    }
    catch( const SceneEngine::SceneException& ex ) {
        ASSERT_EQ( ex.getBadField(), "when" ) << "'when' should be an array";
    }
}

TEST_F( EventParserTest, wrongExceptionsBlock_notArray ) /* 5.2 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = JSON_EMPTY_ARR;
    sceneJson[ "exceptions" ] = SceneEngine::UT::wrongWhenBlockArray;
    try {
        auto eventContainer = parseExceptions( sceneJson );
        FAIL() << "Should be an exception";
    }
    catch( const SceneEngine::SceneException& ex ) {
        ASSERT_EQ( ex.getBadField(), "exceptions" ) << "'exceptions' should be an array";
    }
}

TEST_F( EventParserTest, checkItemEventExpression ) /* 6.1 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { SceneEngine::UT::itemEventExpressionCondition };

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getItemEvents();
    ASSERT_EQ( collection.size(), 1 );
    auto eventPtr = collection.at( 0 ).second;

    auto eventValueKeys = eventPtr->getValueKeys();
    ASSERT_EQ( eventValueKeys.size(), 1 );
    const SceneEngine::ValueSourceKey& eventValueKey = eventValueKeys.front();
    ASSERT_TRUE( eventValueKey.isExpression() );
    EXPECT_EQ( eventValueKey.getSourceId(), "__EXPRESSION_NAME__" );
}

TEST_F( EventParserTest, checkCompareNumbersEventExpression ) /* 6.2 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { SceneEngine::UT::compareNumbersEventExpressionCondition };

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getCompareNumbersEvents();
    ASSERT_EQ( collection.size(), 1 );
    auto eventPtr = collection.at( 0 ).second;

    auto eventValueKeys = eventPtr->getValueKeys();
    ASSERT_EQ( eventValueKeys.size(), 1 );
    const SceneEngine::ValueSourceKey& eventValueKey = eventValueKeys.front();
    ASSERT_TRUE( eventValueKey.isExpression() );
    EXPECT_EQ( eventValueKey.getSourceId(), "__EXPRESSION_NAME__" );
}

TEST_F( EventParserTest, checkMonthWeeks ) /* 7.1 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock = replaceTokens( SceneEngine::UT::isDateWeeksWhenBlockTemplate,
                                    { { "__WEEKS_TYPE__", quotes( "weeks" ) }, { "__WEEKS_ARRAY__", R"([1, -1])" } } );
    sceneJson[ "when" ] = { Json::parse( whenBlock ) };
    LOGGER_TRACE_RAW( cc::j( sceneJson ) );

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.dateRangeEvents;
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, checkYearWeeks ) /* 7.2 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock =
        replaceTokens( SceneEngine::UT::isDateWeeksWhenBlockTemplate,
                       { { "__WEEKS_TYPE__", quotes( "yearWeeks" ) }, { "__WEEKS_ARRAY__", R"([1, 50, -1])" } } );
    sceneJson[ "when" ] = { Json::parse( whenBlock ) };
    LOGGER_TRACE_RAW( cc::j( sceneJson ) );

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.dateRangeEvents;
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, isDateWeeklyEventWithEmptyTimeArray ) /* 7.3 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock = replaceTokens( SceneEngine::UT::isDateWithTimeBlockTemplate,
                                    { { "__TYPE__", "weekly" },
                                      { "__TIME_ARRAY__", "[]" },
                                      { "__DAYS_ARRAY_FIELD__", "weekdays" },
                                      { "__DAYS_ARRAY__", "[2]" } } );
    sceneJson[ "when" ] = { Json::parse( whenBlock ) };
    LOGGER_TRACE_RAW( cc::j( sceneJson ) );

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.dateRangeEvents;
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, isDateMonthlyEventWithEmptyTimeArray ) /* 7.4 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock = replaceTokens( SceneEngine::UT::isDateWithTimeBlockTemplate,
                                    { { "__TYPE__", "monthly" },
                                      { "__TIME_ARRAY__", "[]" },
                                      { "__DAYS_ARRAY_FIELD__", "days" },
                                      { "__DAYS_ARRAY__", "[2]" } } );
    sceneJson[ "when" ] = { Json::parse( whenBlock ) };
    LOGGER_TRACE_RAW( cc::j( sceneJson ) );

    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.dateRangeEvents;
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, checkCompareNumbersRange ) /* 8 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { SceneEngine::UT::compareNumbersRangeCondition };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getCompareNumbersRangeEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, checkSoftwareUpdate ) /* 9 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { SceneEngine::UT::firmwareUpdateStateCondition };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.firmwareUpdateStateEvents;
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, checkFunctionsDurationLess ) /* 10.1.1 */ {
    testFunctionOperatorSuccessCase( R"("for": { "method": "less", "seconds": 20})" );
}

TEST_F( EventParserTest, checkFunctionsDurationLeast ) /* 10.1.2 */ {
    testFunctionOperatorSuccessCase( R"("for": { "method": "least", "seconds": 20})" );
}

TEST_F( EventParserTest, checkFunctionsDurationErrors ) /* 10.1.3 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock = SceneEngine::UT::compareNumbersRangeCondition.dump();
    auto functionOperatorStr =
        replaceTokens( SceneEngine::UT::functionOperatorWhenBlock,
                       { { "__BLOCKS__", std::move( whenBlock ) }, { "__FUNCTION__", R"("anotherKey": {})" } } );
    Json functionOperator = Json::parse( functionOperatorStr );

    sceneJson[ "when" ] = { functionOperator };
    SceneEngine::EventContainer events;

    ASSERT_THROW( events = parseTriggers( sceneJson ), SceneEngine::SceneException );
}

TEST_F( EventParserTest, checkFunctionRepeat ) /* 10.2 */ {
    testFunctionOperatorSuccessCase( R"("repeat": { "times": 3, "seconds": 5})" );
}

TEST_F( EventParserTest, checkFunctionLatch ) /* 10.3 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlock = SceneEngine::UT::compareNumbersRangeCondition.dump();

    const char* badSettings = R"("latch": { "enabled": false })";
    testLatch(
        sceneJson, whenBlock, badSettings, [this]( Json& block, int line ) { shouldThrow( block, line ); }, __LINE__ );

    const char* goodSettings = R"("latch": { "enabled": true })";
    testLatch(
        sceneJson, whenBlock, goodSettings, [this]( Json& block, int line ) { shouldBeOk( block, line ); }, __LINE__ );

    auto twoChildren = whenBlock + ", " + whenBlock;
    testLatch( sceneJson,
               twoChildren,
               goodSettings,
               [this]( Json& block, int line ) { shouldThrow( block, line ); },
               __LINE__ );
}

TEST_F( EventParserTest, checkIsDictionaryChanged ) /* 12 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDictionaryChangedWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__KEY__", "key_1" },
                                           { "__OPERATION__", "added" },
                                       } );

    try {
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        const auto& collection = events.dictionaryChangedEvents;
        ASSERT_EQ( collection.size(), 1 );
        auto eventPtr = collection.at( 0 ).second;
        ASSERT_EQ( eventPtr->getValueKeys().size(), 0 );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, compareStrings ) /* 13 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    KeyStringList substitute = { { "__COMPARE_METHOD__", "compareStrings" },
                                 { "__ITEM_ID__", "aaa" },
                                 { "__COMPARISON__", "!=" },
                                 { "__TYPE__", "string" },
                                 { "__VALUE__", "valueToCompareWith" } };
    auto whenTemplate = replaceTokens( SceneEngine::UT::compareOperatorWhenBlockTemplate, substitute );
    JsonObject whenBlock;
    ASSERT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );

    sceneJson[ "when" ] = { whenBlock };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getCompareStringsEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, stringOperation ) /* 14 */ {
    KeyStringList substitute = { { "__METHOD__", "stringOperation" },
                                 { "__ITEM_ID__", "aaa" },
                                 { "__OPERATION__", SceneEngine::Model::Resources::StringOperations::Begin },
                                 { "__TYPE__", "string" },
                                 { "__VALUE__", R"("operationOnThis")" } };
    auto whenTemplate = replaceTokens( SceneEngine::UT::itemOperationWhenBlockTemplate, substitute );
    JsonObject whenBlock;
    ASSERT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );

    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { whenBlock };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getStringOperationEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneForAnyHotzoneForAnyObject ) /* 15.1 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"([])" },
                                           { "__MATCH__", "any" },
                                           { "__FILTERS__", R"({})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        ASSERT_EQ( events.hotzoneEvents.size(), 1 );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneForAnyHotzoneForAnyFilter ) /* 15.2 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"([])" },
                                           { "__MATCH__", "filter" },
                                           { "__FILTERS__", R"({"objects":{"match":"any"}})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        ASSERT_EQ( events.hotzoneEvents.size(), 1 );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneForAnyHotzoneForAnyOfFilter ) /* 15.3 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"([])" },
                                           { "__MATCH__", "filter" },
                                           { "__FILTERS__", R"({"objects":{"match":"anyOf","list":["objectId1"]}})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        ASSERT_EQ( events.hotzoneEvents.size(), 1 );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneForSpecificHotzonesForAnyFilter ) /* 15.4 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"(["hotzone1", "hotzone2"])" },
                                           { "__MATCH__", "filter" },
                                           { "__FILTERS__", R"({"objects":{"match":"any"}})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        ASSERT_EQ( events.hotzoneEvents.size(), 1 );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneWhenFiltersShouldNotBeProvided ) /* 15.5 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"(["hotzone1", "hotzone2"])" },
                                           { "__MATCH__", "any" },
                                           { "__FILTERS__", R"({"objects":{"match":"any"}})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        EXPECT_THROW( parseTriggers( sceneJson ), SceneEngine::SceneException );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, checkIsDetectedInHotzoneWhenFiltersAreMissing ) /* 15.6 */ {
    Log::setLocalLogLevel( Log::TRACE );
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    auto whenBlockStr = replaceTokens( SceneEngine::UT::isDetectedInHotzoneWhenBlockTemplate,
                                       {
                                           { "__ITEM_ID__", "itemId1" },
                                           { "__HOTZONES__", R"(["hotzone1", "hotzone2"])" },
                                           { "__MATCH__", "filter" },
                                           { "__FILTERS__", R"({})" },
                                       } );

    try {
        LOGGER_DEBUG( whenBlockStr );
        Json whenBlock = Json::parse( whenBlockStr );

        sceneJson[ "when" ] = JSON_ARRAY_( whenBlock );
        LOGGER_DEBUG( cc::j( sceneJson ) );

        EXPECT_THROW( parseTriggers( sceneJson ), SceneEngine::SceneException );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw;
    }
}

TEST_F( EventParserTest, inArrayCorrectData ) /* 16.1 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createInArrayWhenBlock( "array.string", R"(["1", "2", "3"])" ) };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getInArrayEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, inArrayValueNotArray ) /* 16.2 */ {
    const std::string type = "int";

    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createInArrayWhenBlock( type, "1" ) };

    bool exceptionCaught = false;
    try {
        SceneEngine::EventContainer events = parseTriggers( sceneJson );
    }
    catch( const SceneEngine::SceneException& ex ) {
        exceptionCaught = true;
        const std::string expectedMessage = "inArray: value should be an array, actual type: " + type;
        EXPECT_EQ( expectedMessage, ex.what() );
    }

    EXPECT_TRUE( exceptionCaught );
}

TEST_F( EventParserTest, inArrayValueDoesntMatchType ) /* 16.3 */ {
    const std::string type = "array.string";

    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createInArrayWhenBlock( type, "[1, 2, 3]" ) };

    bool exceptionCaught = false;
    try {
        SceneEngine::EventContainer events = parseTriggers( sceneJson );
    }
    catch( const SceneEngine::SceneException& ex ) {
        exceptionCaught = true;
        const std::string expectedMessageStart = "inArray: cannot create valid value: ";
        const std::string exceptionMessage = ex.what();
        EXPECT_EQ( exceptionMessage.find( expectedMessageStart ), 0 );
    }

    EXPECT_TRUE( exceptionCaught );
}

TEST_F( EventParserTest, compareValues_withIsLess ) /* 17.1 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    KeyStringList substitute = { { "__VALUE_TYPE__", "string" },
                                 { "__ITEM_ID__", "aaa" },
                                 { "__COMPARISON__", "!=" },
                                 { "__TYPE__", "string" },
                                 { "__UNQUOTED_VALUE__", R"("valueToCompareWith")" } };
    auto whenTemplate = replaceTokens( SceneEngine::UT::compareValuesWhenBlockTemplate, substitute );
    JsonObject whenBlock;
    ASSERT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );

    sceneJson[ "when" ] = { whenBlock };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getCompareValuesEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, compareValues_withoutIsLess ) /* 17.2 */ {
    static const char* comparatorsEqual[] = { "==", "!=" };
    static const char* comparatorsLess[] = { "<", "<=", ">", ">=" };

    auto checkWithComparator =
        [this]( std::string valueType, std::string comparator, const Json& value, bool shouldThrow, int line ) {
            JsonObject sceneJson = SceneEngine::UT::emptyScene;
            KeyStringList substitute = { { "__ITEM_ID__", "aaa" },
                                         { "__COMPARISON__", comparator },
                                         { "__VALUE_TYPE__", valueType },
                                         { "__TYPE__", valueType },
                                         { "__UNQUOTED_VALUE__", value.dump() } };
            auto whenTemplate = replaceTokens( SceneEngine::UT::compareValuesWhenBlockTemplate, substitute );
            JsonObject whenBlock;
            auto errorAt = std::string{ __FILE__ } + ":" + std::to_string( line );

            ASSERT_NO_THROW( whenBlock = Json::parse( whenTemplate ) ) << errorAt;
            sceneJson[ "when" ] = { whenBlock };
            if( shouldThrow ) {
                try {
                    SceneEngine::EventContainer events = parseTriggers( sceneJson );
                    FAIL() << "Should throw an exception for value " << value.dump() << " at " << errorAt;
                }
                catch( const std::exception& ex ) {
                    Log::debug( "Throws as expected: " + cc::error( ex.what() ) );
                }
            }
            else {
                SceneEngine::EventContainer events = parseTriggers( sceneJson );
                const auto& collection = events.getCompareValuesEvents();

                EXPECT_EQ( collection.size(), 1 ) << errorAt;
            }
        };

    struct TestData {
        std::string valueType;
        Json data;
        int codeLine;
    };
    std::vector< TestData > testData = {
        { "userCode", R"({"code": "some code", "name": "code name", "mode": "disabled" })"_json, __LINE__ },
        { "weekly_interval",
          R"({ "weekDays": [ "monday", "thursday" ], "startTime": "11:50:54", "stopTime": "17:59:59" })"_json,
          __LINE__ },
        { "sound_info", R"({"name": "1AMBUL~1", "duration": 20})"_json, __LINE__ },
        { "user_lock_operation", R"({"action": "unknown", "userId": -1 })"_json, __LINE__ },
    };

    for( const auto& test : testData ) {
        const auto& valueType = test.valueType;
        const auto& value = test.data;
        bool shouldThrow = false;
        for( auto comparator : comparatorsEqual ) {
            checkWithComparator( valueType, comparator, value, shouldThrow, __LINE__ );
        }
        shouldThrow = true;
        for( auto comparator : comparatorsLess ) {
            checkWithComparator( valueType, comparator, value, shouldThrow, test.codeLine );
        }
    }
}

TEST_F( EventParserTest, compareValues_typeMismatchError ) /* 17.3 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    KeyStringList substitute = { { "__VALUE_TYPE__", "token" },
                                 { "__ITEM_ID__", "aaa" },
                                 { "__COMPARISON__", "!=" },
                                 { "__TYPE__", "string" },
                                 { "__UNQUOTED_VALUE__", R"("valueToCompareWith")" } };
    auto whenTemplate = replaceTokens( SceneEngine::UT::compareValuesWhenBlockTemplate, substitute );
    Json whenBlock = Json::parse( whenTemplate );
    ASSERT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );
    sceneJson[ "when" ] = { whenBlock };

    try {
        SceneEngine::EventContainer events = parseTriggers( sceneJson );
        ADD_FAILURE() << "Should throw SCENE_WHEN_TYPE_MISMATCH exception, but it's not";
    }
    catch( const SceneEngine::SceneException& ex ) {
        ASSERT_EQ( ex.getError(), Errors::SCENE_WHEN_TYPE_MISMATCH )
            << "Should throw exception with type mismatch in it";
        LOGGER_TRACE( "Exception: " + ex.what() + ", code: " + json_get_str( Errors::get( ex.getError() ), "data" ) );
    }
}

TEST_F( EventParserTest, followFunction ) /* 18 */ {
    testFunctionOperatorSuccessCase( R"("follow": {"delayReset": 3})" );
}

TEST_F( EventParserTest, pulseFunctionOnlyRequiredParameters ) /* 19.1 */ {
    testFunctionOperatorSuccessCase( R"("pulse": {"truePeriod": 3})" );
}

TEST_F( EventParserTest, pulseFunctionAllParameters ) /* 19.2 */ {
    testFunctionOperatorSuccessCase( R"("pulse": {"truePeriod": 3, "falsePeriod": 3, "times": 3})" );
}

TEST_F( EventParserTest, isItemStateChangedAnyToAny ) /* 20.1 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( R"("any")", R"("any")" ) };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getItemStateChangedEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, isItemStateChangedValueToAny ) /* 20.2 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( "10", R"("any")" ) };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getItemStateChangedEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, isItemStateChangedValueToValue ) /* 20.3 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( "10", "20" ) };
    SceneEngine::EventContainer events = parseTriggers( sceneJson );
    const auto& collection = events.getItemStateChangedEvents();
    ASSERT_EQ( collection.size(), 1 );
}

TEST_F( EventParserTest, isItemStateChangedInvalidCoreDValues ) /* 20.4 */ {
    {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( "10", R"("string2")" ) };
        EXPECT_THROW( parseTriggers( sceneJson ), SceneEngine::SceneException );
    }
    {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( R"("string1")", "10" ) };
        EXPECT_THROW( parseTriggers( sceneJson ), SceneEngine::SceneException );
    }
    {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( R"("string1")", R"("string2")" ) };
        EXPECT_THROW( parseTriggers( sceneJson ), SceneEngine::SceneException );
    }
}

TEST_F( EventParserTest, exceptionsNoErrorsWithoutBlock ) /* 21.1 */ {
    SceneEngine::EventContainer exceptions = parseExceptions( SceneEngine::UT::emptyScene );
    EXPECT_TRUE( exceptions.empty() );
}

TEST_F( EventParserTest, exceptionsEventsParsedCorrectly ) /* 21.2 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "exceptions" ] = { createIsItemStateChangedWhenBlock( "10", "20" ),
                                  createInArrayWhenBlock( "array.string", R"(["1", "2", "3"])" ),
                                  SceneEngine::UT::compareNumbersEventExpressionCondition };

    SceneEngine::EventContainer exceptions = parseExceptions( sceneJson );
    EXPECT_FALSE( exceptions.empty() );
    EXPECT_EQ( exceptions.getInArrayEvents().size(), 1 );
    EXPECT_EQ( exceptions.getCompareNumbersEvents().size(), 1 );
    EXPECT_EQ( exceptions.getItemStateChangedEvents().size(), 1 );
}

TEST_F( EventParserTest, exceptionsParsedAlongsideTriggers ) /* 21.3 */ {
    using SceneEngine::EventContainer;

    JsonObject sceneJson = SceneEngine::UT::emptyScene;
    sceneJson[ "when" ] = { createIsItemStateChangedWhenBlock( "10", "20" ),
                            createInArrayWhenBlock( "array.string", R"(["1", "2", "3"])" ),
                            SceneEngine::UT::compareNumbersEventExpressionCondition };
    sceneJson[ "exceptions" ] = { createIsItemStateChangedWhenBlock( "10", "20" ),
                                  createInArrayWhenBlock( "array.string", R"(["1", "2", "3"])" ),
                                  SceneEngine::UT::compareNumbersEventExpressionCondition };

    EventContainer exceptions = parseExceptions( sceneJson );
    EventContainer triggers = parseExceptions( sceneJson );
    for( const EventContainer& eventContainer : { std::ref( exceptions ), std::ref( triggers ) } ) {
        EXPECT_FALSE( eventContainer.empty() );
        EXPECT_EQ( eventContainer.getInArrayEvents().size(), 1 );
        EXPECT_EQ( exceptions.getCompareNumbersEvents().size(), 1 );
        EXPECT_EQ( eventContainer.getItemStateChangedEvents().size(), 1 );
    }
}

TEST_F( EventParserTest, exceptionsDuplicateIdsBetweenEvents ) /* 21.4 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    // add the same placeholder to all events
    JsonObject block1 = createIsItemStateChangedWhenBlock( "10", "20" );
    block1[ "_id" ] = "${placeholder-1}";
    Json block2 = createInArrayWhenBlock( "array.string", R"(["1", "2", "3"])" );
    block2[ "_id" ] = "${placeholder-1}";
    Json block3 = SceneEngine::UT::compareNumbersEventExpressionCondition;
    block2[ "_id" ] = "${placeholder-1}";
    sceneJson[ "exceptions" ] = JSON_ARRAY_( block1, block2, block3 );

    SceneEngine::EventContainer exceptions;
    ASSERT_THROW( exceptions = parseExceptions( sceneJson ), SceneEngine::SceneException )
        << "Should throw on multiple events with the same ID";
}

TEST_F( EventParserTest, exceptionsDuplicateIdsBetweenOperatorAndEvents ) /* 21.5 */ {
    JsonObject sceneJson = SceneEngine::UT::emptyScene;

    // add the same placeholder to an operator and event
    const char* idPlaceholder = "${placeholder-1}";
    JsonObject block = createIsItemStateChangedWhenBlock( "10", "20" );
    block[ "_id" ] = idPlaceholder;

    KeyStringList substitute = { { "__OPERATOR__", "and" }, { "__BLOCKS__", block.dump() } };
    auto whenTemplate = replaceTokens( SceneEngine::UT::logicalOperatorWhenBlock, substitute );
    JsonObject whenBlock;
    EXPECT_NO_THROW( whenBlock = Json::parse( whenTemplate ) );
    whenBlock[ "_id" ] = idPlaceholder;

    sceneJson[ "exceptions" ] = JSON_ARRAY_( whenBlock );

    SceneEngine::EventContainer exceptions;
    ASSERT_THROW( exceptions = parseExceptions( sceneJson ), SceneEngine::SceneException )
        << "Should throw on multiple events with the same ID";
}

TEST_F( EventParserTest, sunStateRangeEvent ) /* 22.1 */ {
    try {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        sceneJson[ "when" ] = JSON_ARRAY_( SceneEngine::UT::sunstateConditionRange );
        auto eventContainer = parseTriggers( sceneJson );
        ASSERT_FALSE( eventContainer.sunStateEvents.empty() );
        ASSERT_TRUE( eventContainer.sunStateEvents.at( 0 )->isRangeEvent() );
    }
    catch( const std::exception& ex ) {
        FAIL() << "Exception caught: " << ex.what();
    }
}

TEST_F( EventParserTest, sunStateTimePointEvent ) /* 22.2 */ {
    try {
        JsonObject sceneJson = SceneEngine::UT::emptyScene;
        sceneJson[ "when" ] = JSON_ARRAY_( SceneEngine::UT::sunstateCondition );
        auto eventContainer = parseTriggers( sceneJson );
        ASSERT_FALSE( eventContainer.sunStateEvents.empty() );
        ASSERT_FALSE( eventContainer.sunStateEvents.at( 0 )->isRangeEvent() );
    }
    catch( const std::exception& ex ) {
        FAIL() << "Exception caught: " << ex.what();
    }
}

TEST_F( EventParserTest, constructorExceptions ) /* 23 */ {
    using namespace ScenesManager;

    auto geoPositionGetter = []() { return HubPlatform::CoreDRemoteInterface::GeoPosition{ 49.99450, 36.21506 }; };
    auto clocks = std::make_shared< SceneEngine::TestClock >( std::chrono::seconds( 0 ), std::chrono::hours( 2 ) );
    auto clockGetter = [clocks]() mutable -> SceneEngine::IClock& { return *clocks.get(); };

    Services::TaskLoop taskLoop;
    std::unique_ptr< EventParserInterface > parser;

    EXPECT_NO_THROW(
        parser = EventParserFactory::makeEventParser( "id", geoPositionGetter, clockGetter, nullptr, taskLoop ) );
    EXPECT_THROW( parser = EventParserFactory::makeEventParser( "id", nullptr, clockGetter, nullptr, taskLoop ),
                  std::invalid_argument )
        << "empty geo-position getter should throw";
    EXPECT_THROW( parser = EventParserFactory::makeEventParser( "id", geoPositionGetter, nullptr, nullptr, taskLoop ),
                  std::invalid_argument )
        << "empty clock getter should throw";
}

}  // namespace UT
}  // namespace HubPlatform
