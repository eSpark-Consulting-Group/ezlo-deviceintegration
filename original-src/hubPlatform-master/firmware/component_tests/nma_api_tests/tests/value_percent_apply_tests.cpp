#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/exceptions.h"
#include "cored/model/values/values.h"
#include "jsonrpc/jsonrpc.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include <database_client/dbqueryparser.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1. Set a device with temperature item, current, min and max values
DONE    2. Calculate new values using remote interface's getItemValueInMinMaxRange() method.
DONE    2.0 Combine different (current, min, max) item properties in different scales.
DONE    2.1 Positive percent value
DONE    2.2 Negative percent value
DONE    2.3 Convert result to celsius in test to check actual calculation value

*/

using namespace CoreD;
using namespace Services;

class ItemConfiguration;
using TestCase = std::tuple< ItemConfiguration, float >;

class EFWR1893_ValuePercentApplyTests : public CoreNmaTest, public ::testing::WithParamInterface< TestCase > {
public:
    EFWR1893_ValuePercentApplyTests()
    : remoteInterface( makeRemote() ) {}

    void SetUp() override;

    void TearDown() override;

protected:
    void addDevice( int codeLine );
    void dropDevice( int codeLine );
    void addItem( Model::ValueType valueType, const JsonObject& currentValue, int codeLine );
    void updateItemMinMax( const Model::ValueType& valueType,
                           const JsonObject& currentValue,
                           const JsonObject& minValue,
                           const JsonObject& maxValue,
                           int codeLine );
    void dropItem( int codeLine );

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;
    std::string m_deviceId;
    std::string m_itemId;
};

void EFWR1893_ValuePercentApplyTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );

    addDevice( __LINE__ );
}

void EFWR1893_ValuePercentApplyTests::TearDown() {
    // dropDevice
    dropDevice( __LINE__ );

    CoreNmaTest::TearDown();
}

// addDevice : m_deviceId
void EFWR1893_ValuePercentApplyTests::addDevice( int codeLine ) {
    JsonObject broadcast;

    const std::string gatewayId = getMainGateway( remoteInterface, __LINE__ );

    Model::Device thermostat( gatewayId, "Z-Wave Thermostat", "thermostat", "139_21586_21570", "hvac", false );
    thermostat.setSubcategory( "hvac" );
    auto thermostatAddResult = remoteInterface.addDevice( thermostat );
    EXPECT_FALSE( thermostatAddResult.first.empty() );
    EXPECT_TRUE( thermostatAddResult.second );

    m_deviceId = thermostatAddResult.first;

    broadcast = waitForBroadcast( "hub.device.added",
                                  std::string( __FILE__ ) + ":" + std::to_string( codeLine ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
}  // addDevice

void EFWR1893_ValuePercentApplyTests::dropDevice( int codeLine ) {
    JsonObject broadcast;

    remoteInterface.removeDevice( m_deviceId );
    if( !m_itemId.empty() ) {
        broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( codeLine ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    }
    broadcast = waitForBroadcast( "hub.device.removed",
                                  std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
    m_deviceId.clear();
}

void EFWR1893_ValuePercentApplyTests::dropItem( int codeLine ) {
    if( m_itemId.empty() )
        return;

    remoteInterface.removeItem( m_itemId );
    auto broadcast = waitForBroadcast( "hub.item.removed", "code line: " + std::to_string( codeLine ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    m_itemId.clear();
}

// addItem(ValueType = Temperature, itemConf.current) -> m_itemId
void EFWR1893_ValuePercentApplyTests::addItem( Model::ValueType valueType,
                                               const JsonObject& currentValue,
                                               int codeLine ) {
    Model::ValueUPtr value = Model::ValueFactory::createItemValue( valueType, currentValue );

    Model::Item setpointItem( m_deviceId, "thermostat_setpoint", valueType, true, true, std::move( value ) );
    auto setpointAddResult = remoteInterface.addItem( setpointItem );
    m_itemId = setpointAddResult.first;

    EXPECT_FALSE( setpointAddResult.first.empty() );
    EXPECT_TRUE( setpointAddResult.second );
    auto broadcast = waitForBroadcast( "hub.item.added",
                                       std::string( __FILE__ ) + ":" + std::to_string( codeLine ) + " - no broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" );
}

void EFWR1893_ValuePercentApplyTests::updateItemMinMax( const Model::ValueType& valueType,
                                                        const JsonObject& currentValue,
                                                        const JsonObject& minValue,
                                                        const JsonObject& maxValue,
                                                        int codeLine ) {  //
    auto makeValueLoader = []( const Json& source ) -> CoreDRemoteInterface::ValueLoader {
        return [&source]( CoreD::Model::ValueType valueType ) {
            return CoreD::Model::ValueFactory::createItemValue( valueType, source );
        };
    };

    remoteInterface.updateItemValueWithMinMax(
        m_itemId, makeValueLoader( currentValue ), makeValueLoader( minValue ), makeValueLoader( maxValue ) );

    auto broadcast = waitForBroadcast( "hub.item.updated",
                                       std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - no broadcast" );
    auto broadcastDump = broadcast.dump(), currentValueDump = currentValue.dump();  // dbg
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" ) << broadcastDump << "\n";
    JsonObject result, resValueMin, resValueMax;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "name" ), "thermostat_setpoint" );
    EXPECT_EQ( json_get_str( result, "_id" ), m_itemId );
    // TODO: efwr-1975; change valueMin to minValue
    EXPECT_EQ( result.count( "valueMin" ), 0 ) << "deprecated field, minValue should be instead: " << broadcastDump;
    EXPECT_EQ( result.count( "valueMax" ), 0 ) << "deprecated field, maxValue should be instead: " << broadcastDump;
    EXPECT_EQ( result.count( "minValue" ), 1 ) << "field not found: minValue, dump: " << broadcastDump;
    EXPECT_EQ( result.count( "maxValue" ), 1 ) << "field not found: maxValue, dump: " << broadcastDump;

    auto& conf = remoteInterface.getConfiguration();
    auto defaultScale = conf.getScaleForValueType( valueType );

    auto convert = [this, &valueType]( float srcValue, std::string from ) -> float {
        auto& conf = remoteInterface.getConfiguration();
        auto defaultScale = conf.getScaleForValueType( valueType );
        if( from.empty() )
            from = defaultScale;
        auto to = defaultScale;
        Model::ValueConverter converter = valueType.getConverter();
        if( from != to )
            return converter( srcValue, from.c_str(), to.c_str() );
        return srcValue;
    };

    auto convMinValue = convert( json_get_float( minValue, "value" ), json_get_str( minValue, "scale" ) );
    EXPECT_FLOAT_EQ( json_get_float( result, "minValue" ), convMinValue )
        << result.dump() << ", minValue: " << minValue.dump();
    auto convMaxValue = convert( json_get_float( maxValue, "value" ), json_get_str( maxValue, "scale" ) );
    EXPECT_FLOAT_EQ( json_get_float( result, "maxValue" ), convMaxValue );
}

struct ItemConfiguration {
public:
    using ValueUPtr = CoreD::Model::ValueUPtr;
    using SharedUPtr = std::shared_ptr< CoreD::Model::Value >;

    SharedUPtr m_current, m_minValue, m_maxValue;

public:
    ItemConfiguration() = default;
    ItemConfiguration( ValueUPtr& current, ValueUPtr& minValue, ValueUPtr& maxValue )
    : m_current( current.release(), current.get_deleter() )
    , m_minValue( minValue.release(), minValue.get_deleter() )
    , m_maxValue( maxValue.release(), maxValue.get_deleter() ) {}
    ItemConfiguration( std::initializer_list< JsonObject > lst ) {
        setItemConf( *this, lst );
    }

    static void uniqueToShared( SharedUPtr& to, ValueUPtr& from ) {
        to.reset( from.release(), from.get_deleter() );
    }

    static void setItemConf( ItemConfiguration& itemConf, std::initializer_list< JsonObject > initList ) {
        auto makeTemperatureValue = []( JsonObject const& json ) {
            auto dump = json.dump();  // DBG
            auto uptr = CoreD::Model::ValueFactory::createItemValue( CoreD::Model::ValueType::Enum::Temperature, json );

            //        itemConf.m_current.reset(uptrCurrent.release(), uptrCurrent.get_deleter() );
            return std::move( uptr );
        };

        if( initList.size() < 3 )
            throw std::runtime_error( "Initializer list is too short, 3 elements expected!" );

        using ValueUPtr = CoreD::Model::ValueUPtr;
        ValueUPtr newCurrent = makeTemperatureValue( *( initList.begin() + 0 ) );
        ItemConfiguration::uniqueToShared( itemConf.m_current, newCurrent );
        ValueUPtr newMin = makeTemperatureValue( *( initList.begin() + 1 ) );
        ItemConfiguration::uniqueToShared( itemConf.m_minValue, newMin );
        ValueUPtr newMax = makeTemperatureValue( *( initList.begin() + 2 ) );
        ItemConfiguration::uniqueToShared( itemConf.m_maxValue, newMax );
    };

    friend void PrintTo( ItemConfiguration const&, std::ostream* os );
};

void PrintTo( ItemConfiguration const& conf, std::ostream* os ) {
    *os << " current: " << conf.m_current->toJson().dump() << ", minValue: " << conf.m_minValue->toJson().dump()
        << ", maxValue: " << conf.m_maxValue->toJson().dump();
}

namespace {
using InitList = std::initializer_list< JsonObject >;
InitList initList0 = { JSON( "valueType"
                             << "temperature"
                             << "value" << 25 << "scale" << Model::ValueScales::Celsius ),
                       JSON( "valueType"
                             << "temperature"
                             << "value" << 0 << "scale" << Model::ValueScales::Celsius ),
                       JSON( "valueType"
                             << "temperature"
                             << "value" << 40 << "scale" << Model::ValueScales::Celsius ) };

InitList initList1 = { JSON( "valueType"
                             << "temperature"
                             << "value" << 80 << "scale" << Model::ValueScales::Fahrenheit ),
                       JSON( "valueType"
                             << "temperature"
                             << "value" << 0 << "scale" << Model::ValueScales::Fahrenheit ),
                       JSON( "valueType"
                             << "temperature"
                             << "value" << 218 << "scale" << Model::ValueScales::Fahrenheit ) };

InitList initListMixedScales = { JSON( "valueType"
                                       << "temperature"
                                       << "value" << 80 << "scale" << Model::ValueScales::Fahrenheit ),
                                 JSON( "valueType"
                                       << "temperature"
                                       << "value" << 0 << "scale" << Model::ValueScales::Celsius ),
                                 JSON( "valueType"
                                       << "temperature"
                                       << "value" << 40 << "scale" << Model::ValueScales::Celsius ) };

InitList initListMixedScalesBack = { JSON( "valueType"
                                           << "temperature"
                                           << "value" << 25 << "scale" << Model::ValueScales::Celsius ),
                                     JSON( "valueType"
                                           << "temperature"
                                           << "value" << 32 << "scale" << Model::ValueScales::Fahrenheit ),  // 0 C
                                     JSON( "valueType"
                                           << "temperature"
                                           << "value" << 104 << "scale" << Model::ValueScales::Fahrenheit ) };  // 40 C

const auto configurations = ::testing::Values( initListMixedScales, initListMixedScalesBack, initList0, initList1 );

const auto percents = ::testing::Values( 2, 10, -2, -10 );

struct Minus {
public:
    Minus( float v )
    : m_v( v ) {}

    friend std::ostream& operator<<( std::ostream& ss, Minus const& );

private:
    float m_v{ 0 };
};

std::ostream& operator<<( std::ostream& ss, Minus const& m ) {
    if( m.m_v < 0 ) {
        ss << std::string( "Minus" ) << fabs( m.m_v );
    }
    else {
        ss << m.m_v;
    }
    return ss;
}

std::string testNamePrinter( const testing::TestParamInfo< EFWR1893_ValuePercentApplyTests::ParamType >& info ) {
    ItemConfiguration const& itemConf = std::get< 0 >( info.param );
    float percent = std::get< 1 >( info.param );

    std::stringstream ss;
    Model::ScalableValue* minValPtr = dynamic_cast< Model::ScalableValue* >( itemConf.m_minValue.get() );
    Model::ScalableValue* maxValPtr = dynamic_cast< Model::ScalableValue* >( itemConf.m_maxValue.get() );
    Model::ScalableValue* curValPtr = dynamic_cast< Model::ScalableValue* >( itemConf.m_current.get() );

    if( nullptr == minValPtr || nullptr == maxValPtr || nullptr == curValPtr )
        throw std::runtime_error( "Could not convert to scalable" );

    ss << "Percent" << Minus( percent ) << "min" << Minus( minValPtr->getValue() ) << "scale" << minValPtr->getScale()
       << "max" << Minus( maxValPtr->getValue() ) << "scale" << maxValPtr->getScale() << "cur"
       << Minus( curValPtr->getValue() ) << "scale" << curValPtr->getScale();

    auto str = ss.str();
    return std::move( str );
};

}  // namespace

INSTANTIATE_TEST_CASE_P( CombineScalesAndPercents,
                         EFWR1893_ValuePercentApplyTests,
                         ::testing::Combine(
                             // Item configuration
                             configurations,
                             // Percents for hub.item.value.percent.apply command
                             percents ),
                         testNamePrinter );

TEST_P( EFWR1893_ValuePercentApplyTests, getItemValueInMinMaxRange ) {
    auto testParams = GetParam();
    ItemConfiguration const& itemConf = std::get< 0 >( testParams );
    float percent = std::get< 1 >( testParams );
    int codeLine = __LINE__;  // TODO: get line from test params

    // drop item
    dropItem( __LINE__ );

    ASSERT_TRUE( itemConf.m_current != nullptr );
    ASSERT_TRUE( itemConf.m_minValue != nullptr );
    ASSERT_TRUE( itemConf.m_maxValue != nullptr );

    auto current = itemConf.m_current->toJson();
    auto minValue = itemConf.m_minValue->toJson();
    auto maxValue = itemConf.m_maxValue->toJson();

    // create new item, set its min/max values
    addItem( Model::ValueType::Enum::Temperature, current, codeLine );
    updateItemMinMax( Model::ValueType::Enum::Temperature, current, minValue, maxValue, codeLine );

    // run test
    using ScalableValue = CoreD::Model::ScalableValue;
    auto valueUPtr = remoteInterface.getItemValueInMinMaxRange( m_itemId, percent );
    ASSERT_FALSE( valueUPtr == nullptr );

    auto scalable = dynamic_cast< ScalableValue* >( valueUPtr.get() );
    ASSERT_FALSE( scalable == nullptr );

    // Convert all to celsius here, in visitor conversions are made with main value's scale terms!
    const char* baseScale = Model::ValueScales::Celsius;
    float setpointValueF = dynamic_cast< ScalableValue* >( itemConf.m_current.get() )->getConvertedValue( baseScale );
    float minValueF = dynamic_cast< ScalableValue* >( itemConf.m_minValue.get() )->getConvertedValue( baseScale );
    float maxValueF = dynamic_cast< ScalableValue* >( itemConf.m_maxValue.get() )->getConvertedValue( baseScale );
    auto resultingValue = scalable->getConvertedValue( baseScale );

    auto expectedValue = setpointValueF + ( maxValueF - minValueF ) * percent / 100.0f;
    ASSERT_FLOAT_EQ( resultingValue, expectedValue ) << "percent: " << percent << ", min: " << minValue.dump()
                                                     << ", max: " << maxValue.dump() << ", current: " << current.dump();
}

class ItemValueMinMaxRenameTests : public CoreNmaTest {};

}  // namespace Tests
}  // namespace HubPlatform
