#include "nma_api_tests/application.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/values.h"
#include "cored/exceptions.h"
#include "jsonrpc/jsonrpc.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:
    1. Set item value for some types for itemId specified
    2. Set item value for some types for array of items

*/

using namespace CoreD;
using namespace Services;

using MakeRequest =
    std::function< JsonObject( std::string const& itemId, JsonObject const& value, JsonObject const& scale ) >;
using AssertionPred = std::function< void( std::string itemId, JsonObject const& value, int codeLine ) >;

JsonObject makeRequestDefault( std::string const& itemId, JsonObject const& value, JsonObject const& scale );

class SetItemValueTypeFixture : public CoreNmaTest {
public:
    SetItemValueTypeFixture();
    static void SetUpTestCase();
    static void TearDownTestCase();

    void SetUp() override;
    void TearDown() override;

    void TestBody() override{};

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;

    using ItemNameToItemIdMap = std::map< std::string, std::string >;
    static ItemNameToItemIdMap& getItemNameToItemIdMap() {
        static ItemNameToItemIdMap dict;
        return dict;
    }
};

SetItemValueTypeFixture::SetItemValueTypeFixture()
: remoteInterface( makeRemote( nma() ) ) {}

void SetItemValueTypeFixture::SetUpTestCase() {
    Log::trace( "testSuite: SetUp" );

    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    SetItemValueTypeFixture coreTest;
    coreTest.CommonNmaTests::SetUp();

    // create device
    auto deviceId = coreTest.addDevice( remoteInterface,
                                        "testDevice",
                                        "smoke",
                                        "134_3_24",
                                        "security_sensor",
                                        "smoke",
                                        false /*batteryPowered*/,
                                        __LINE__ );

    ItemNameToItemIdMap& itemNameToItemId = getItemNameToItemIdMap();

    // create items for test
    using namespace Model::Resources;

    {
        std::string itemName = "item_int";
        int value = 1;
        auto itemId = coreTest.addItem(
            remoteInterface, deviceId, itemName, CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
        auto setItemValueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
        EXPECT_NO_THROW( remoteInterface.setItemValue( setItemValueRequest ) );
        coreTest.checkItemUpdateBroadcast( itemId, value, __LINE__ );
        itemNameToItemId[ itemName ] = itemId;
    }

    {
        std::string itemName = "item_float";
        float value = 1.5;
        auto itemId = coreTest.addItem(
            remoteInterface, deviceId, itemName, CoreD::Model::ValueType::Enum::Float, true, true, __LINE__ );
        auto setItemValueRequest = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
        EXPECT_NO_THROW( remoteInterface.setItemValue( setItemValueRequest ) );
        coreTest.checkItemUpdateBroadcast( itemId, value, __LINE__ );
        itemNameToItemId[ itemName ] = itemId;
    }

    {
        std::string itemName = "item_temperature";
        float value = 10.0f;
        auto itemId = coreTest.addItem(
            remoteInterface, deviceId, itemName, CoreD::Model::ValueType::Enum::Temperature, true, true, __LINE__ );
        auto setItemValueRequest =
            JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value << "scale"
                                          << "celsius" );
        EXPECT_NO_THROW( remoteInterface.setItemValue( setItemValueRequest ) );

        auto broadcast = coreTest.waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "hub.item.updated" ) );
        Json result;
        ASSERT_TRUE( json_get_obj( broadcast, "result", result ) ) << "Invalid broadcast";
        coreTest.compareTemperatureValues(
            setItemValueRequest, result, "value", NMA_TEST_MAKE_MESSAGE( "Temperature values should match" ) );

        itemNameToItemId[ itemName ] = itemId;
    }
    coreTest.CommonNmaTests::TearDown();
}

void SetItemValueTypeFixture::TearDownTestCase() {
    Log::trace( "testSuite: TearDown" );
    SetItemValueTypeFixture coreTest;

    // Call it to wait of all broadcasts passed after the tests done
    coreTest.CommonNmaTests::SetUp();

    coreTest.clearExistingDevices( __LINE__ );
    getItemNameToItemIdMap().clear();
}

void SetItemValueTypeFixture::SetUp() {
    Log::trace( "testCase: SetUp" );
    CoreNmaTest::SetUp();
}

void SetItemValueTypeFixture::TearDown() {
    Log::trace( "testCase: SetUp" );
    CoreNmaTest::TearDown();
}

struct TestCaseItemValueSet {
    std::string m_itemName;
    JsonObject m_valueToSet;
    bool m_shouldThrow;
};

void PrintTo( const TestCaseItemValueSet& test, std::ostream* os ) {
    *os << test.m_itemName << ": " << test.m_valueToSet.dump();
    if( test.m_shouldThrow )
        *os << ", should throw";
}

class SetItemValueTypeTest : public SetItemValueTypeFixture,
                             public ::testing::WithParamInterface< TestCaseItemValueSet > {
public:
    void SetUp() override {
        SetItemValueTypeFixture::SetUp();
        Log::trace( "SetItemValueTypeTest::SetUp" );
    }
    void TearDown() override {
        SetItemValueTypeFixture::TearDown();
        Log::trace( "SetItemValueTypeTest::TearDown" );
    }

    static void SetUpTestCase() {
        Log::trace( "testSuite: SetUpTestCase" );
        SetItemValueTypeFixture::SetUpTestCase();
    }
    static void TearDownTestCase() {
        Log::trace( "testSuite: TearDownTestCase" );
        SetItemValueTypeFixture::TearDownTestCase();
    }

    void callSetItemValue( std::string const& itemId,
                           JsonObject const& params,
                           MakeRequest requestLoader = makeRequestDefault,
                           AssertionPred assertionPred = nullptr );

protected:
    void setItemValue( const std::string& itemId,
                       const JsonObject& value,
                       int codeLine,
                       JsonObject scale,
                       MakeRequest requestLoader = makeRequestDefault,
                       AssertionPred assertionPred = nullptr );

protected:
    std::string m_lastError;
};

JsonObject makeRequestDefault( std::string const& itemId, JsonObject const& value, JsonObject const& scale ) {
    using namespace CoreD::Model::Resources;
    auto request = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
    if( !scale.empty() ) {
        request[ "scale" ] = scale;
    }
    return request;
}

JsonObject makeRequestMulticast( std::string const& itemId, JsonObject const& value, JsonObject const& scale ) {
    using namespace CoreD::Model::Resources;
    JsonObject items = JSON_ARRAY( itemId );
#ifdef _DEBUG
    auto dump = items.dump();
#endif
    auto request = JSON( "ids" << items << ItemFields::ValueFieldName << value );
    if( !scale.empty() ) {
        request[ "scale" ] = scale;
    }
    return request;
}

void SetItemValueTypeTest::setItemValue( const std::string& itemId,
                                         const JsonObject& value,
                                         int codeLine,
                                         JsonObject scale,
                                         MakeRequest requestLoader,
                                         AssertionPred assertionPred ) {
    using namespace CoreD::Model::Resources;
    auto request = requestLoader( itemId, value, scale );
#ifdef _DEBUG
    auto dumpRequest = request.dump();
#endif
    remoteInterface.setItemValue( request );

    if( assertionPred != nullptr )
        assertionPred( itemId, value, codeLine );
}

void SetItemValueTypeTest::callSetItemValue( std::string const& itemName,
                                             JsonObject const& params,
                                             MakeRequest requestLoader,
                                             AssertionPred assertionPred ) {
    using namespace Model;
    m_lastError.clear();

    auto& itemNameToItemIdMap = getItemNameToItemIdMap();
    ASSERT_FALSE( itemNameToItemIdMap.empty() ) << "Invalid test configuration: no devices set up!";

    auto foundIt = itemNameToItemIdMap.find( itemName );

    ASSERT_FALSE( foundIt == itemNameToItemIdMap.end() ) << std::string( "Item not found by name '" + itemName + "'" );

    // All checks are performed there:
    std::string const& itemId = foundIt->second;
#ifdef _DEBUG
    auto paramsDump = params.dump();
#endif
    auto scale = params[ "scale" ];

    // default assert callback, works well in case of no errors and no value conversions needed
    try {
        this->setItemValue( itemId, params[ "value" ], __LINE__, scale, requestLoader, assertionPred );
    }
    catch( InvalidValueScale const& e ) {
        m_lastError = e.what();
        throw e;
    }
    catch( CoreDModelError const& e ) {
        m_lastError = e.what();
        throw e;
    }
    catch( std::exception const& e ) {
        m_lastError = e.what();
        throw e;
    }
}

TEST_P( SetItemValueTypeTest, SetItemByType ) {
    Log::trace( "setItemByType" );

    auto test = GetParam();
    const std::string& itemName = test.m_itemName;
    const JsonObject& request = test.m_valueToSet;

    auto assertCallback = [this, request]( std::string itemId, JsonObject const& value, int codeLine ) {
        using namespace Model;
        float toCheck = value.get< float >();
        auto scale = json_get_str( request, "scale" );

        if( !scale.empty() ) {
            ValueType valueType = ValueType::Enum::Temperature;
            auto const& defaultScale = remoteInterface.getConfiguration().getScaleForValueType( valueType );
            ValueUPtr value = ValueFactory::createItemValue( valueType, request );
            if( defaultScale.empty() == false && value.get() != nullptr ) {
                ItemValueFloatVisitor toFloat( defaultScale.c_str() );
                value->accept( toFloat );
                toCheck = toFloat.getValue();
            }
        }
        return this->checkItemUpdateBroadcast( itemId, toCheck, codeLine );
    };

    auto loader = makeRequestDefault;
    if( test.m_shouldThrow ) {
        ASSERT_THROW( callSetItemValue( itemName, request, loader, assertCallback ), Model::InvalidValueScale )
            << "Error thrown: " << m_lastError;
    }
    else {
        ASSERT_NO_THROW( callSetItemValue( itemName, request, loader, assertCallback ) )
            << "Error thrown: " << m_lastError;
    }
}

class SetItemValueMulticastTypeTest : public SetItemValueTypeTest {};

TEST_P( SetItemValueMulticastTypeTest, SetMulticaseByType ) {
    Log::trace( "SetSingleItemInArrayByType" );

    auto test = GetParam();
    const std::string& itemName = test.m_itemName;
    const JsonObject& request = test.m_valueToSet;

    auto assertCallback = [this, &request]( std::string itemId, JsonObject const& value, int codeLine ) {
        using namespace Model;
        float toCheck = value.get< float >();
        auto scale = json_get_str( request, "scale" );

        if( !scale.empty() ) {
            ValueType valueType = ValueType::Enum::Temperature;
            auto const& defaultScale = remoteInterface.getConfiguration().getScaleForValueType( valueType );
            ValueUPtr value = ValueFactory::createItemValue( valueType, request );
            if( defaultScale.empty() == false && value.get() != nullptr ) {
                ItemValueFloatVisitor toFloat( defaultScale.c_str() );
                value->accept( toFloat );
                toCheck = toFloat.getValue();
            }
        }
        return this->checkItemUpdateBroadcast( itemId, toCheck, codeLine );
    };

    auto loader = makeRequestMulticast;
    if( test.m_shouldThrow ) {
        ASSERT_THROW( callSetItemValue( itemName, request, loader, assertCallback ), Model::InvalidValueScale )
            << "Error thrown: " << m_lastError;
    }
    else {
        ASSERT_NO_THROW( callSetItemValue( itemName, request, loader, assertCallback ) )
            << "Error thrown: " << m_lastError;
    }
}

using PrinterPred = std::string( const testing::TestParamInfo< TestCaseItemValueSet >& );

static unsigned nextPrinterCounter() {
    static unsigned counter = 0;
    return counter++;
}

std::string stringParamTestSuffix( const testing::TestParamInfo< TestCaseItemValueSet >& info ) {
    return std::string( info.param.m_itemName ) + "N" + std::to_string( nextPrinterCounter() );
};

static auto testValues = ::testing::Values( TestCaseItemValueSet{ "item_int", JSON( "value" << 2 ), false },
                                            TestCaseItemValueSet{ "item_float", JSON( "value" << 2.222f ), false },
                                            TestCaseItemValueSet{ "item_temperature", JSON( "value" << 20 ), false },
                                            TestCaseItemValueSet{ "item_temperature",
                                                                  JSON( "value" << 21 << "scale"
                                                                                << "" ),
                                                                  false },
                                            TestCaseItemValueSet{ "item_temperature",
                                                                  JSON( "value" << 22 << "scale"
                                                                                << "celsius" ),
                                                                  false },
                                            TestCaseItemValueSet{ "item_temperature",
                                                                  // 25 Celsius
                                                                  JSON( "value" << 77 << "scale"
                                                                                << "fahrenheit" ),
                                                                  false },
                                            TestCaseItemValueSet{ "item_temperature",
                                                                  JSON( "value" << 26 << "scale"
                                                                                << "invalid_value" ),
                                                                  true } );

/** To work, values should differ from item's current value! */
INSTANTIATE_TEST_CASE_P( SetValueByType, SetItemValueTypeTest, testValues, stringParamTestSuffix );

INSTANTIATE_TEST_CASE_P( SetValueByType, SetItemValueMulticastTypeTest, testValues, stringParamTestSuffix );

TEST_F( SetItemValueMulticastTypeTest, SetDifferentItems ) {
    Json items = JSON_EMPTY_OBJ;
    auto& itemNameToItemIdMap = getItemNameToItemIdMap();
    auto itemId1 = itemNameToItemIdMap[ "item_int" ];
    items[ itemId1 ] = JSON( "value" << 2 );

    auto itemId2 = itemNameToItemIdMap[ "item_float" ];
    items[ itemId2 ] = JSON( "value" << 2.222f );

    auto itemId3 = itemNameToItemIdMap[ "item_temperature" ];
    items[ itemId3 ] = JSON( "value" << 22 << "scale"
                                     << "celsius" );

    Json request = JSON( "items" << std::move( items ) );
    remoteInterface.setItemValue( request );

    checkBroadcasts(
        { [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, 2 ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, 2.222f ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 22 ); } },
        __LINE__ );
}

TEST_F( SetItemValueMulticastTypeTest, SetDifferentItemsWrongId ) {
    Json items = JSON_EMPTY_OBJ;
    auto& itemNameToItemIdMap = getItemNameToItemIdMap();
    auto itemId1 = itemNameToItemIdMap[ "item_int" ];
    items[ itemId1 ] = JSON( "value" << 2 );

    items[ "wrong_id" ] = JSON( "value" << 2.222f );

    auto itemId3 = itemNameToItemIdMap[ "item_temperature" ];
    items[ itemId3 ] = JSON( "value" << 22 << "scale"
                                     << "celsius" );

    Json request = JSON( "items" << std::move( items ) );
    ASSERT_THROW( remoteInterface.setItemValue( request ), HubPlatform::CoreD::ItemNotFoundError );
}

}  // namespace Tests
}  // namespace HubPlatform
