#include "extensions_manager/extensions/plugins/gateway/gateway_config.h"
#include "json_utils/json_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class GatewayConfigTestSuite : public ::testing::Test {
protected:
    static Json getDefaultJsonConfig() {
        return JSON( "gateway" << JSON( "name"
                                        << "myplug"
                                        << "label"
                                        << "test label"
                                        << "reason"
                                        << "test reason"
                                        << "setItemValueCommand"
                                        << "setValue"
                                        << "setItemValueResponsePolicy"
                                        << "custom"
                                        << "addItemDictionaryValueCommand"
                                        << "addDictionaryValue"
                                        << "setItemDictionaryValueCommand"
                                        << "setDictionaryValue"
                                        << "removeItemDictionaryValueCommand"
                                        << "removeDictionaryValue"
                                        << "clearItemDictionaryCommand"
                                        << "clearDictionary"
                                        << "forceRemoveDeviceCommand"
                                        << "forceRemoveDevice"
                                        << "unreachableReasons"
                                        << JSON_ARRAY( JSON( "reason"
                                                             << "r1"
                                                             << "advice"
                                                             << "a1" )
                                                       << JSON( "reason"
                                                                << "r2"
                                                                << "advice"
                                                                << "a2" ) )
                                        << "unreachableActions"
                                        << JSON_ARRAY( JSON( "label"
                                                             << "l1"
                                                             << "method"
                                                             << "m1" )
                                                       << JSON( "label"
                                                                << "l2"
                                                                << "method"
                                                                << "m2" ) )
                                        << "operations"
                                        << JSON_ARRAY( JSON( "operation"
                                                             << "add_device"
                                                             << "template"
                                                             << "zwave:swart_adding.template" )
                                                       << JSON( "operation"
                                                                << "remove_device"
                                                                << "template"
                                                                << "zwave:smart_removing.template" ) ) ) );
    }
};

TEST_F( GatewayConfigTestSuite, createFromJson ) {
    const auto config = GatewayConfig::getGatewayConfig( getDefaultJsonConfig() );

    ASSERT_TRUE( config );

    EXPECT_EQ( config->m_name, "myplug" );
    EXPECT_EQ( config->m_label, "test label" );
    EXPECT_EQ( config->m_reason, "test reason" );
    EXPECT_EQ( config->m_setItemValueCommand, "setValue" );
    EXPECT_EQ( config->m_setItemValueResponsePolicy, "custom" );
    EXPECT_EQ( config->m_addItemDictionaryValueCommand, "addDictionaryValue" );
    EXPECT_EQ( config->m_setItemDictionaryValueCommand, "setDictionaryValue" );
    EXPECT_EQ( config->m_removeItemDictionaryValueCommand, "removeDictionaryValue" );
    EXPECT_EQ( config->m_clearItemDictionaryCommand, "clearDictionary" );
    EXPECT_THAT( config->m_unreachableActions,
                 ::testing::ElementsAre( GatewayConfig::GatewayUnreachableActions::value_type{ "l1", "m1" },
                                         GatewayConfig::GatewayUnreachableActions::value_type{ "l2", "m2" } ) );
    EXPECT_THAT( config->m_unreachableReasons,
                 ::testing::ElementsAre( GatewayConfig::GatewayUnreachableReasons::value_type{ "r1", "a1" },
                                         GatewayConfig::GatewayUnreachableReasons::value_type{ "r2", "a2" } ) );
    EXPECT_THAT(
        config->m_operations,
        ::testing::ElementsAre( GatewayConfig::Operation{ "add_device", "zwave:swart_adding.template" },
                                GatewayConfig::Operation{ "remove_device", "zwave:smart_removing.template" } ) );
}

TEST_F( GatewayConfigTestSuite, createFromJsonWithoutNecessaryFields ) {
    auto testWithRemovedField = []( const char* field ) {
        auto json = getDefaultJsonConfig();
        json[ "gateway" ].erase( field );
        const auto config = GatewayConfig::getGatewayConfig( json );

        EXPECT_FALSE( config ) << "Gateway should not be created without " << field;
    };

    testWithRemovedField( "name" );
    testWithRemovedField( "label" );
    testWithRemovedField( "setItemValueCommand" );
}

TEST_F( GatewayConfigTestSuite, createFromJsonWithEmptyNecessaryFields ) {
    auto testWithEmptyStringField = []( const char* field ) {
        auto json = getDefaultJsonConfig();
        json[ "gateway" ][ field ] = "";
        const auto config = GatewayConfig::getGatewayConfig( json );

        EXPECT_FALSE( config );
    };

    testWithEmptyStringField( "name" );
    testWithEmptyStringField( "label" );
    testWithEmptyStringField( "setItemValueCommand" );
}

TEST_F( GatewayConfigTestSuite, createFromJsonWithoutOptionalFields ) {
    auto json = getDefaultJsonConfig();
    json[ "gateway" ].erase( "operations" );
    // Since PR-1326 the fields reason, unreachableActions and unreachableReasons are optional
    json[ "gateway" ].erase( "reason" );
    json[ "gateway" ].erase( "unreachableReasons" );
    json[ "gateway" ].erase( "unreachableActions" );

    const auto config = GatewayConfig::getGatewayConfig( json );

    ASSERT_TRUE( config );

    EXPECT_TRUE( config->m_operations.empty() );
    EXPECT_EQ( config->m_name, "myplug" );
}

TEST_F( GatewayConfigTestSuite, createFromJsonWithEmptyOrBrokenOptionalFields ) {
    {
        auto json = getDefaultJsonConfig();
        json[ "gateway" ][ "operations" ] = Json();
        const auto config = GatewayConfig::getGatewayConfig( json );

        ASSERT_TRUE( config );

        EXPECT_TRUE( config->m_operations.empty() );
        EXPECT_EQ( config->m_name, "myplug" );
    }

    {
        auto json = getDefaultJsonConfig();
        json[ "gateway" ][ "operations" ] = JSON( "name"
                                                  << "1" );
        const auto config = GatewayConfig::getGatewayConfig( json );

        ASSERT_TRUE( config );

        EXPECT_TRUE( config->m_operations.empty() );
        EXPECT_EQ( config->m_name, "myplug" );
    }
}

TEST_F( GatewayConfigTestSuite, createFromJsonWithToLongValue ) {
    auto json = getDefaultJsonConfig();

    auto testWithTooLongField = []( const char* field ) {
        auto json = getDefaultJsonConfig();
        json[ "gateway" ][ field ] =
            "This field is too long. The length is more 128 symbols. We use the restriction to "
            "avoid creation of unlimited fields by user. May be the limit should be changed.";
        const auto config = GatewayConfig::getGatewayConfig( json );

        EXPECT_FALSE( config );
    };

    testWithTooLongField( "name" );
    testWithTooLongField( "label" );
    testWithTooLongField( "setItemValueCommand" );
}

TEST_F( GatewayConfigTestSuite, createFromEmptyJson ) {
    EXPECT_FALSE( GatewayConfig::getGatewayConfig( {} ) );
}

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
