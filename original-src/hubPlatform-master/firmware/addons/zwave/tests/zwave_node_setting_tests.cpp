#include "zwave/zwave_node_setting.h"

#include "database_client/tests/idbclient_mock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Creation
DONE        1.1 From empty settings
DONE        1.2 Not existing setting
DONE        1.3 One setting
DONE        1.4 Several settings
DONE    2 Setting content
DONE        2.1 Configuration
DONE            2.1.1 No configuration
DONE            2.1.2 One configuration
DONE            2.1.3 Several configurations
DONE        2.2 Association
DONE            2.2.1 No association
DONE            2.2.1 One association
DONE            2.2.1 Several associations
DONE        2.3 One element for each type
DONE        2.4 Several elements for each type
DONE        2.5 Wake up interval
DONE        2.6 Should request notifications
DONE        2.7 Extended user codes request restricted
DONE        2.8 Database settings
DONE            2.8.1 Configurations
DONE                2.8.1.1 Single database configuration
DONE                2.8.1.2 Several database configuration
DONE                2.8.1.3 With simple configuration
DONE                2.8.1.4 Overrides
DONE            2.8.2 Associations
DONE                2.8.2.1 Single database association
DONE                2.8.2.2 Several database association
DONE                2.8.2.3 With simple association
DONE            2.8.3 Mixed
DONE        2.9 Ignore CC intrrview

*/

namespace znet {
static bool operator==( const Configuration& lhs, const Configuration& rhs ) {
    return std::tie( lhs.paramNumber, lhs.size, lhs.value ) == std::tie( rhs.paramNumber, rhs.size, rhs.value );
}

static bool operator==( const Association& lhs, const Association& rhs ) {
    return std::tie( lhs.channelId, lhs.groupId, lhs.targetChannelId ) ==
           std::tie( rhs.channelId, rhs.groupId, lhs.targetChannelId );
}

static void PrintTo( const Configuration& config, ::std::ostream* os ) {
    *os << "{ "
        << ", paramNumber: " << static_cast< int >( config.paramNumber )
        << ", size: " << static_cast< int >( config.size ) << ", value: " << static_cast< int >( config.value ) << " }";
}

static void PrintTo( const Association& association, ::std::ostream* os ) {
    *os << "{ "
        << "channelId: " << static_cast< int >( association.channelId )
        << ", groupId: " << static_cast< int >( association.groupId )
        << ", targetChannelId: " << static_cast< int >( association.targetChannelId ) << " }";
}
}  // namespace znet

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

class NodeSettingTestSuite : public Test {
public:
    using Configurations = std::vector< znet::Configuration >;
    using Associations = std::vector< znet::Association >;

public:
    Configurations fetchConfigurations( const znet::INodeSetting& setting ) const {
        Configurations result;
        setting.forEachConfiguration( [&result]( const znet::Configuration& config ) { result.push_back( config ); } );
        return result;
    }

    Associations fetchAssociations( const znet::INodeSetting& setting ) const {
        Associations result;
        setting.forEachAssociation(
            [&result]( const znet::Association& association ) { result.push_back( association ); } );
        return result;
    }

protected:
    HubPlatform::UT::IDbClientMock m_dbClient;
};

TEST_F( NodeSettingTestSuite, testCreationFromEmptySettings /*1.1*/ ) {
    const Json settings = R"({})"_json;

    ASSERT_FALSE( NodeSetting::create( settings, m_dbClient, 0, 0, 0 ) );
}

TEST_F( NodeSettingTestSuite, testCreationNotExistingSetting /*1.2*/ ) {
    const Json settings = R"({
        "1_1_1": {}
    })"_json;

    ASSERT_FALSE( NodeSetting::create( settings, m_dbClient, 0, 0, 0 ) );
}

TEST_F( NodeSettingTestSuite, testCreationOneSetting /*1.3*/ ) {
    const Json settings = R"({
        "1_1_1": {}
    })"_json;

    ASSERT_TRUE( NodeSetting::create( settings, m_dbClient, 1, 1, 1 ) );
}

TEST_F( NodeSettingTestSuite, testCreationSeveralSettings /*1.4*/ ) {
    const Json settings = R"({
        "1_1_1": {},
        "2_2_2": {},
        "3_3_3": {}
    })"_json;

    EXPECT_TRUE( NodeSetting::create( settings, m_dbClient, 1, 1, 1 ) );
    EXPECT_TRUE( NodeSetting::create( settings, m_dbClient, 2, 2, 2 ) );
    EXPECT_TRUE( NodeSetting::create( settings, m_dbClient, 3, 3, 3 ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentConfigurationNoConfiguration /*2.1.1*/ ) {
    const Json settings = R"({
        "1_1_1": {}
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( fetchConfigurations( *setting ).empty() );
}

TEST_F( NodeSettingTestSuite, testSettingContentConfigurationOneConfiguration /*2.1.2*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 80,
                    "size": 1,
                    "value": 2
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config;
    config.paramNumber = 80;
    config.value = 2;
    config.size = 1;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentConfigurationSeveralConfigurations /*2.1.3*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 1,
                    "size": 2,
                    "value": 3
                },
                {
                    "number": 5,
                    "value": 7
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config1;
    config1.paramNumber = 1;
    config1.value = 3;
    config1.size = 2;

    znet::Configuration config2;
    config2.paramNumber = 5;
    config2.value = 7;
    config2.size = 4;

    ASSERT_THAT( fetchConfigurations( *setting ), ContainerEq( Configurations{ { config1, config2 } } ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentAssociationNoAssociation /*2.2.1*/ ) {
    const Json settings = R"({
        "1_1_1": {}
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( fetchAssociations( *setting ).empty() );
}

TEST_F( NodeSettingTestSuite, testSettingContentAssociationOneAssociation /*2.2.2*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "associations": [
                {
                    "channel_id": 0,
                    "group_id": 1
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Association associotion;
    associotion.channelId = 0;
    associotion.groupId = 1;
    associotion.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ElementsAre( associotion ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentAssociationSeveralAssociation /*2.2.3*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "associations": [
                {
                    "group_id": 1,
                    "target_channel_id": 0
                },
                {
                    "channel_id": 2,
                    "group_id": 3
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Association associotion1;
    associotion1.channelId = 0;
    associotion1.groupId = 1;
    associotion1.targetChannelId = 0;

    znet::Association associotion2;
    associotion2.channelId = 2;
    associotion2.groupId = 3;
    associotion2.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ContainerEq( Associations{ associotion1, associotion2 } ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentOneElementForEachType /*2.3*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 1,
                    "size": 2,
                    "value": 3
                }
            ],
            "associations": [
                {
                    "channel_id": 4,
                    "group_id": 5
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config;
    config.paramNumber = 1;
    config.value = 3;
    config.size = 2;

    znet::Association associotion;
    associotion.channelId = 4;
    associotion.groupId = 5;
    associotion.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    EXPECT_THAT( fetchConfigurations( *setting ), ElementsAre( config ) );
    EXPECT_THAT( fetchAssociations( *setting ), ElementsAre( associotion ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentSeveralElementsForEachType /*2.4*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 1,
                    "size": 2,
                    "value": 3
                },
                {
                    "number": 5,
                    "size": 6,
                    "value": 7
                }
            ],
            "associations": [
                {
                    "channel_id": 0,
                    "group_id": 1
                },
                {
                    "channel_id": 2,
                    "group_id": 3
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config1;
    config1.paramNumber = 1;
    config1.value = 3;
    config1.size = 2;

    znet::Configuration config2;
    config2.paramNumber = 5;
    config2.value = 7;
    config2.size = 6;

    znet::Association associotion1;
    associotion1.channelId = 0;
    associotion1.groupId = 1;
    associotion1.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    znet::Association associotion2;
    associotion2.channelId = 2;
    associotion2.groupId = 3;
    associotion2.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    EXPECT_THAT( fetchConfigurations( *setting ), ContainerEq( Configurations{ { config1, config2 } } ) );
    EXPECT_THAT( fetchAssociations( *setting ), ContainerEq( Associations{ { associotion1, associotion2 } } ) );
}

TEST_F( NodeSettingTestSuite, testSettingContentWakeUpInterval /*2.5*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "wake_up_interval": 7200
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( setting->getWakeUpInterval().has_value() );
    ASSERT_EQ( *setting->getWakeUpInterval(), 7200 );
}

TEST_F( NodeSettingTestSuite, testSettingContentShouldRequestNotifications /*2.6*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "should_request_notifications": false
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( setting->shouldRequestNotifications().has_value() );
    ASSERT_EQ( *setting->shouldRequestNotifications(), false );
}

TEST_F( NodeSettingTestSuite, testSettingContentExtendedUserCodesRequestRestricted /*2.7*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "supported_user_codes": {
                "extended_user_codes_request_restricted": true
            }
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( setting->isExtendedUserCodesRequestRestricted() );
    ASSERT_TRUE( *setting->isExtendedUserCodesRequestRestricted() );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsConfigurationSingle /*2.8.1.1*/ ) {
    const Json dbSettings = R"({
        "configurations": [
            {
                "number": 80,
                "size": 1,
                "value": 2
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    auto setting = NodeSetting::create( JSON_EMPTY_OBJ, m_dbClient, 1, 1, 1 );

    znet::Configuration config;
    config.paramNumber = 80;
    config.value = 2;
    config.size = 1;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsConfigurationSeveral /*2.8.1.2*/ ) {
    const Json dbSettings = R"({
        "configurations": [
            {
                "number": 80,
                "size": 1,
                "value": 2
            },
            {
                "number": 81,
                "size": 4,
                "value": 15
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    auto setting = NodeSetting::create( JSON_EMPTY_OBJ, m_dbClient, 1, 1, 1 );

    znet::Configuration config1;
    config1.paramNumber = 80;
    config1.value = 2;
    config1.size = 1;

    znet::Configuration config2;
    config2.paramNumber = 81;
    config2.value = 15;
    config2.size = 4;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config1, config2 ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsConfigurationWithSimple /*2.7.1.3*/ ) {
    const Json dbSettings = R"({
        "configurations": [
            {
                "number": 80,
                "size": 1,
                "value": 2
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 81,
                    "size": 4,
                    "value": 15
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config1;
    config1.paramNumber = 80;
    config1.value = 2;
    config1.size = 1;

    znet::Configuration config2;
    config2.paramNumber = 81;
    config2.value = 15;
    config2.size = 4;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config1, config2 ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsConfigurationOverrides /*2.8.1.4*/ ) {
    const Json dbSettings = R"({
        "configurations": [
            {
                "number": 80,
                "size": 1,
                "value": 2
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 80,
                    "size": 4,
                    "value": 15
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Configuration config1;
    config1.paramNumber = 80;
    config1.value = 2;
    config1.size = 1;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config1 ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsAssociationSingle /*2.8.2.1*/ ) {
    const Json dbSettings = R"({
        "associations": [
            {
                "channel_id": 0,
                "group_id": 1
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    auto setting = NodeSetting::create( JSON_EMPTY_OBJ, m_dbClient, 1, 1, 1 );

    znet::Association associotion;
    associotion.channelId = 0;
    associotion.groupId = 1;
    associotion.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ElementsAre( associotion ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsAssociationSeveral /*2.8.2.2*/ ) {
    const Json dbSettings = R"({
        "associations": [
            {
                "channel_id": 0,
                "group_id": 1
            },
            {
                "channel_id": 0,
                "group_id": 2
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    auto setting = NodeSetting::create( JSON_EMPTY_OBJ, m_dbClient, 1, 1, 1 );

    znet::Association associotion1;
    associotion1.channelId = 0;
    associotion1.groupId = 1;
    associotion1.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    znet::Association associotion2;
    associotion2.channelId = 0;
    associotion2.groupId = 2;
    associotion2.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ElementsAre( associotion1, associotion2 ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsAssociationWithSimple /*2.8.2.3*/ ) {
    const Json dbSettings = R"({
        "associations": [
            {
                "channel_id": 0,
                "group_id": 1
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    const Json settings = R"({
        "1_1_1": {
            "associations": [
                {
                    "channel_id": 0,
                    "group_id": 2
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    znet::Association associotion1;
    associotion1.channelId = 0;
    associotion1.groupId = 1;
    associotion1.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    znet::Association associotion2;
    associotion2.channelId = 0;
    associotion2.groupId = 2;
    associotion2.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ElementsAre( associotion1, associotion2 ) );
}

TEST_F( NodeSettingTestSuite, testDatabaseSettingsMixed /*2.8.3*/ ) {
    const Json dbSettings = R"({
        "configurations": [
            {
                "number": 80,
                "size": 1,
                "value": 2
            },
            {
                "number": 81,
                "size": 1,
                "value": 3
            }
        ],
        "associations": [
            {
                "channel_id": 0,
                "group_id": 1
            }
        ]
    })"_json;

    EXPECT_CALL( m_dbClient, findAll ).WillOnce( Return( std::vector< Json >{ dbSettings } ) );

    const Json settings = R"({
        "1_1_1": {
            "configurations": [
                {
                    "number": 80,
                    "size": 4,
                    "value": 15
                },
                {
                    "number": 82,
                    "size": 4,
                    "value": 15
                }
            ],
            "associations": [
                {
                    "channel_id": 0,
                    "group_id": 2
                }
            ]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    // Check Configurations
    znet::Configuration config1;
    config1.paramNumber = 80;
    config1.value = 2;
    config1.size = 1;

    znet::Configuration config2;
    config2.paramNumber = 81;
    config2.value = 3;
    config2.size = 1;

    znet::Configuration config3;
    config3.paramNumber = 82;
    config3.value = 15;
    config3.size = 4;

    ASSERT_THAT( fetchConfigurations( *setting ), ElementsAre( config1, config2, config3 ) );

    // Check Associations
    znet::Association associotion1;
    associotion1.channelId = 0;
    associotion1.groupId = 1;
    associotion1.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    znet::Association associotion2;
    associotion2.channelId = 0;
    associotion2.groupId = 2;
    associotion2.targetChannelId = ZNET_CHANNEL_ID_INVALID;

    ASSERT_THAT( fetchAssociations( *setting ), ElementsAre( associotion1, associotion2 ) );
}

TEST_F( NodeSettingTestSuite, testIgnoreCcIntervew /*2.9*/ ) {
    const Json settings = R"({
        "1_1_1": {
            "ignore_cc_interview": [15, 16]
        }
    })"_json;

    auto setting = NodeSetting::create( settings, m_dbClient, 1, 1, 1 );

    ASSERT_TRUE( setting->ignoreCcInterciew( 15 ) );
    ASSERT_TRUE( setting->ignoreCcInterciew( 16 ) );

    ASSERT_FALSE( setting->ignoreCcInterciew( 1 ) );
    ASSERT_FALSE( setting->ignoreCcInterciew( 17 ) );
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
