#include "test_utils/component_tests/common_nma_test_fixture.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. hub.addons.zwave.nodes.provisioning.add
DONE        1.1 Add new
DONE        1.2 Override
DONE        1.3 No required parameters
DONE        1.4 Invalid parameter format
TODO        1.5 Entries limit
DONE        1.6 Override with the same parameters
DONE        1.7 Supported adding modes
DONE        1.8 Incorrect adding mode
DONE    2. hub.addons.zwave.nodes.provisioning.set
DONE        2.1 Change fields
DONE        2.2 No required parameters
DONE        2.3 Invalid parameter format
DONE        2.4 No entry found
DONE        2.5 Change fields to the same values
DONE        2.6 Incorrect adding mode
DONE    3. hub.addons.zwave.nodes.provisioning.remove
DONE        3.1 Remove entries
DONE        3.2 No required parameters
DONE        3.3 Invalid parameter format
DONE        3.4 No entry found
DONE    4. hub.addons.zwave.nodes.provisioning.list
DONE        4.1 No filters
TODO        4.2 Filter by node ids
DONE        4.3 Filter by provisioning ids
DONE    5. zwave.get_provisioning_info
DONE        5.1 Get existing
DONE        5.2 Get missing
DONE    6. zwave.set_provisioning_info
DONE        5.1 Set existing
DONE        5.2 Set missing

*/

namespace HubPlatform {
namespace Tests {

class ProvisioningListTests : public CommonNmaTests {
public:
    void addDefaultProvisionEntry(
        const std::vector< uint16_t >& dsk = { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 } ) {
        Json request = PARSE_JSON( "{\n"
                                   "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                                   "    \"id\": \"_ID_\",\n"
                                   "    \"params\": {\n"
                                   "        \"product\": {\n"
                                   "            \"manufacturer\": 20,\n"
                                   "            \"type\": 20,\n"
                                   "            \"id\": 30,\n"
                                   "            \"version\": 1,\n"
                                   "            \"subVersion\": 0\n"
                                   "        },\n"
                                   "        \"maxRequestInterval\": 10,\n"
                                   "        \"name\": \"test\",\n"
                                   "        \"room\": \"none\",\n"
                                   "        \"addingMode\": \"smart\"\n"
                                   "    }\n"
                                   "}" );

        request[ "params" ][ "dsk" ] = dsk;

        auto answer = nma().sendtoHubAndGetAnswer( request );
        EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
        EXPECT_TRUE( answer[ "result" ].empty() );

        waitForAnyOfBroadcasts(
            { "hub.addons.zwave.nodes.provisioning.added", "hub.addons.zwave.nodes.provisioning.updated" },
            getCodeLineString( __LINE__ ) );
    }

    void removeProvisioningEntry( const std::string& id = "1490156181" ) {
        Json request = PARSE_JSON( "{\n"
                                   "    \"method\": \"hub.addons.zwave.nodes.provisioning.remove\",\n"
                                   "    \"id\": \"_ID_\",\n"
                                   "    \"params\": {\n"
                                   "        \"provisioning\": []\n"
                                   "    }\n"
                                   "}" );

        request[ "params" ][ "provisioning" ].push_back( id );

        auto answer = nma().sendtoHubAndGetAnswer( request );
        EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
        EXPECT_TRUE( answer[ "result" ].empty() );

        NMA_TEST_WAIT_FOR_BROADCAST( "hub.addons.zwave.nodes.provisioning.removed" );
    }
};

// resources
static char securityTestScript[] = "HUB:security_test/scripts/security_test";

TEST_F( ProvisioningListTests, ProvisioningAddAddNew /* 1.1 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"none\",\n"
                               "        \"addingMode\": \"smart\"\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.addons.zwave.nodes.provisioning.added" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.added\",\n"
               "    \"result\": {\n"
               "        \"provisioning\": [\n"
               "            {\n"
               "                \"_id\": \"1490156181\",\n"
               "                \"addingMode\": \"smart\",\n"
               "                \"dsk\": [\n"
               "                    58975,\n"
               "                    37056,\n"
               "                    29754,\n"
               "                    11588,\n"
               "                    18902,\n"
               "                    12511,\n"
               "                    34338,\n"
               "                    34338\n"
               "                ],\n"
               "                \"maxRequestInterval\": 10,\n"
               "                \"name\": \"test\",\n"
               "                \"node\": 0,\n"
               "                \"product\": {\n"
               "                    \"id\": 30,\n"
               "                    \"manufacturer\": 20,\n"
               "                    \"subVersion\": 0,\n"
               "                    \"type\": 20,\n"
               "                    \"version\": 1\n"
               "                },\n"
               "                \"room\": \"none\",\n"
               "                \"securityModes\": [],\n"
               "                \"status\": \"notDetected\",\n"
               "                \"supportedAddingModes\": [\n"
               "                    \"s2\",\n"
               "                    \"smart\"\n"
               "                ],\n"
               "                \"supportedModes\": [\n"
               "                    \"accessControl\",\n"
               "                    \"authenticated\",\n"
               "                    \"unauthenticated\",\n"
               "                    \"s0\"\n"
               "                ]\n"
               "            }\n"
               "        ]\n"
               "    }\n"
               "}",
               broadcast.dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningAddOverride /* 1.2 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"kitchen\",\n"
                               "        \"addingMode\": \"smart\"\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.addons.zwave.nodes.provisioning.updated" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.updated\",\n"
               "    \"result\": {\n"
               "        \"provisioning\": [\n"
               "            {\n"
               "                \"_id\": \"1490156181\",\n"
               "                \"room\": \"kitchen\"\n"
               "            }\n"
               "        ]\n"
               "    }\n"
               "}",
               broadcast.dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningAddNoRequiredParameters /* 1.3 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32600,\n"
               "    \"data\": \"rpc.params.notfound.name\",\n"
               "    \"message\": \"Bad request, name does not exist\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
}

TEST_F( ProvisioningListTests, ProvisioningAddInvalidParameterFormat /* 1.4 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588 ]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.dsk\",\n"
               "    \"message\": \"Value is not in valid range\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
}

TEST_F( ProvisioningListTests, ProvisioningAddOverrideWithSameParameters /* 1.6 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"none\",\n"
                               "        \"addingMode\": \"smart\"\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    NMA_TEST_WAIT_NO_BROADCASTS();

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningAddWithSupportedAddingModes /* 1.7 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"none\",\n"
                               "        \"addingMode\": \"s2\",\n"
                               "        \"supportedAddingModes\": [\"s2\"]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.addons.zwave.nodes.provisioning.added" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.added\",\n"
               "    \"result\": {\n"
               "        \"provisioning\": [\n"
               "            {\n"
               "                \"_id\": \"1490156181\",\n"
               "                \"addingMode\": \"s2\",\n"
               "                \"dsk\": [\n"
               "                    58975,\n"
               "                    37056,\n"
               "                    29754,\n"
               "                    11588,\n"
               "                    18902,\n"
               "                    12511,\n"
               "                    34338,\n"
               "                    34338\n"
               "                ],\n"
               "                \"maxRequestInterval\": 10,\n"
               "                \"name\": \"test\",\n"
               "                \"node\": 0,\n"
               "                \"product\": {\n"
               "                    \"id\": 30,\n"
               "                    \"manufacturer\": 20,\n"
               "                    \"subVersion\": 0,\n"
               "                    \"type\": 20,\n"
               "                    \"version\": 1\n"
               "                },\n"
               "                \"room\": \"none\",\n"
               "                \"securityModes\": [],\n"
               "                \"status\": \"notDetected\",\n"
               "                \"supportedAddingModes\": [\n"
               "                    \"s2\"\n"
               "                ],\n"
               "                \"supportedModes\": [\n"
               "                    \"accessControl\",\n"
               "                    \"authenticated\",\n"
               "                    \"unauthenticated\",\n"
               "                    \"s0\"\n"
               "                ]\n"
               "            }\n"
               "        ]\n"
               "    }\n"
               "}",
               broadcast.dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningAddInvalidAddingMode /* 1.8 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"none\",\n"
                               "        \"addingMode\": \"smart\",\n"
                               "        \"supportedAddingModes\": [\"s2\"]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.addingMode\",\n"
               "    \"message\": \"Value is not in valid range\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
}

TEST_F( ProvisioningListTests, ProvisioningSetChangeFields /* 2.1 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"_id\": \"1490156181\",\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 5,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": null\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.addons.zwave.nodes.provisioning.updated" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.updated\",\n"
               "    \"result\": {\n"
               "        \"provisioning\": [\n"
               "            {\n"
               "                \"_id\": \"1490156181\",\n"
               "                \"maxRequestInterval\": null,\n"
               "                \"product\": {\n"
               "                    \"version\": 5\n"
               "                }\n"
               "            }\n"
               "        ]\n"
               "    }\n"
               "}",
               broadcast.dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningSetNoRequiredParameters /* 2.2 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"_id\": \"1490156181\",\n"
                               "        \"product\": {\n"
                               "            \"version\": 15\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32600,\n"
               "    \"data\": \"rpc.params.notfound.product.manufacturer\",\n"
               "    \"message\": \"Bad request, product.manufacturer does not exist\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningSetInvalidParameterFormat /* 2.3 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"_id\": \"1490156181\",\n"
                               "        \"name\": null\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.name\",\n"
               "    \"message\": \"Value is not in valid range\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningSetNoEntryFound /* 2.4 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"_id\": \"404\",\n"
                               "        \"product\": {\n"
                               "            \"version\": 25\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32500,\n"
               "    \"data\": \"generic.object.not.found\",\n"
               "    \"message\": \"Object not found\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningSetChangeFieldsToTheSameValue /* 2.5 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"_id\": \"1490156181\",\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    NMA_TEST_WAIT_NO_BROADCASTS();

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningSetIncorrectAddingMode /* 2.6 */ ) {
    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.add\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"dsk\": [ 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 ],\n"
                               "        \"product\": {\n"
                               "            \"manufacturer\": 20,\n"
                               "            \"type\": 20,\n"
                               "            \"id\": 30,\n"
                               "            \"version\": 1,\n"
                               "            \"subVersion\": 0\n"
                               "        },\n"
                               "        \"maxRequestInterval\": 10,\n"
                               "        \"name\": \"test\",\n"
                               "        \"room\": \"none\",\n"
                               "        \"addingMode\": \"s2\",\n"
                               "        \"supportedAddingModes\": [\"s2\"]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    NMA_TEST_WAIT_FOR_BROADCAST( "hub.addons.zwave.nodes.provisioning.added" );

    request = PARSE_JSON( "{\n"
                          "    \"method\": \"hub.addons.zwave.nodes.provisioning.set\",\n"
                          "    \"id\": \"_ID_\",\n"
                          "    \"params\": {\n"
                          "        \"_id\": \"1490156181\",\n"
                          "        \"addingMode\": \"smart\"\n"
                          "    }\n"
                          "}" );

    answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.addingMode\",\n"
               "    \"message\": \"Value is not in valid range\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningRemoveRemoveEntries /* 3.1 */ ) {
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 } );
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34339 } );

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.remove\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"provisioning\": [ \"1490156181\", \"1490156180\" ]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );
    EXPECT_TRUE( answer[ "result" ].empty() );

    checkBroadcasts( { []( const Json& broadcast ) {
                          const std::string expected =
                              "{\n"
                              "    \"id\": \"ui_broadcast\",\n"
                              "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.removed\",\n"
                              "    \"result\": {\n"
                              "        \"provisioning\": [\n"
                              "            \"1490156181\"\n"
                              "        ]\n"
                              "    }\n"
                              "}";
                          Json broadcastTmp = broadcast;
                          broadcastTmp.erase( "msg_id" );
                          return broadcastTmp.dump( 4 ) == expected;
                      },
                       []( const Json& broadcast ) {
                           const std::string expected =
                               "{\n"
                               "    \"id\": \"ui_broadcast\",\n"
                               "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.removed\",\n"
                               "    \"result\": {\n"
                               "        \"provisioning\": [\n"
                               "            \"1490156180\"\n"
                               "        ]\n"
                               "    }\n"
                               "}";
                           Json broadcastTmp = broadcast;
                           broadcastTmp.erase( "msg_id" );
                           return broadcastTmp.dump( 4 ) == expected;
                       } },
                     __LINE__ );
}

TEST_F( ProvisioningListTests, ProvisioningRemoveNoRequiredParameters /* 3.2 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.remove\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32600,\n"
               "    \"data\": \"rpc.params.notfound.provisioning\",\n"
               "    \"message\": \"Bad request, provisioning does not exist\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningRemoveInvalidParameterFormat /* 3.3 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.remove\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"provisioning\": \"1490156181\"\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.provisioning\",\n"
               "    \"message\": \"Value is not in valid range\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningRemoveNoEntryFound /* 3.4 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.remove\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"provisioning\": [ \"404\" ]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "result" ].empty() );

    EXPECT_EQ( "{\n"
               "    \"code\": -32500,\n"
               "    \"data\": \"generic.object.not.found\",\n"
               "    \"message\": \"Object not found\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ProvisioningListNoFilters /* 4.1 */ ) {
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 } );
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34339 } );

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.list\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {}\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    EXPECT_EQ( "{\n"
               "    \"provisioning\": [\n"
               "        {\n"
               "            \"_id\": \"1490156180\",\n"
               "            \"addingMode\": \"smart\",\n"
               "            \"dsk\": [\n"
               "                58975,\n"
               "                37056,\n"
               "                29754,\n"
               "                11588,\n"
               "                18902,\n"
               "                12511,\n"
               "                34338,\n"
               "                34339\n"
               "            ],\n"
               "            \"maxRequestInterval\": 10,\n"
               "            \"name\": \"test\",\n"
               "            \"node\": 0,\n"
               "            \"product\": {\n"
               "                \"id\": 30,\n"
               "                \"manufacturer\": 20,\n"
               "                \"subVersion\": 0,\n"
               "                \"type\": 20,\n"
               "                \"version\": 1\n"
               "            },\n"
               "            \"room\": \"none\",\n"
               "            \"securityModes\": [],\n"
               "            \"status\": \"notDetected\",\n"
               "            \"supportedAddingModes\": [\n"
               "                \"s2\",\n"
               "                \"smart\"\n"
               "            ],\n"
               "            \"supportedModes\": [\n"
               "                \"accessControl\",\n"
               "                \"authenticated\",\n"
               "                \"unauthenticated\",\n"
               "                \"s0\"\n"
               "            ]\n"
               "        },\n"
               "        {\n"
               "            \"_id\": \"1490156181\",\n"
               "            \"addingMode\": \"smart\",\n"
               "            \"dsk\": [\n"
               "                58975,\n"
               "                37056,\n"
               "                29754,\n"
               "                11588,\n"
               "                18902,\n"
               "                12511,\n"
               "                34338,\n"
               "                34338\n"
               "            ],\n"
               "            \"maxRequestInterval\": 10,\n"
               "            \"name\": \"test\",\n"
               "            \"node\": 0,\n"
               "            \"product\": {\n"
               "                \"id\": 30,\n"
               "                \"manufacturer\": 20,\n"
               "                \"subVersion\": 0,\n"
               "                \"type\": 20,\n"
               "                \"version\": 1\n"
               "            },\n"
               "            \"room\": \"none\",\n"
               "            \"securityModes\": [],\n"
               "            \"status\": \"notDetected\",\n"
               "            \"supportedAddingModes\": [\n"
               "                \"s2\",\n"
               "                \"smart\"\n"
               "            ],\n"
               "            \"supportedModes\": [\n"
               "                \"accessControl\",\n"
               "                \"authenticated\",\n"
               "                \"unauthenticated\",\n"
               "                \"s0\"\n"
               "            ]\n"
               "        }\n"
               "    ]\n"
               "}",
               answer[ "result" ].dump( 4 ) );

    removeProvisioningEntry( "1490156180" );
    removeProvisioningEntry( "1490156181" );
}

TEST_F( ProvisioningListTests, ProvisioningListFilterByProvisioningId /* 4.3 */ ) {
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34338 } );
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34339 } );

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.addons.zwave.nodes.provisioning.list\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {"
                               "        \"ids\": [ \"1490156180\" ]\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    EXPECT_EQ( "{\n"
               "    \"provisioning\": [\n"
               "        {\n"
               "            \"_id\": \"1490156180\",\n"
               "            \"addingMode\": \"smart\",\n"
               "            \"dsk\": [\n"
               "                58975,\n"
               "                37056,\n"
               "                29754,\n"
               "                11588,\n"
               "                18902,\n"
               "                12511,\n"
               "                34338,\n"
               "                34339\n"
               "            ],\n"
               "            \"maxRequestInterval\": 10,\n"
               "            \"name\": \"test\",\n"
               "            \"node\": 0,\n"
               "            \"product\": {\n"
               "                \"id\": 30,\n"
               "                \"manufacturer\": 20,\n"
               "                \"subVersion\": 0,\n"
               "                \"type\": 20,\n"
               "                \"version\": 1\n"
               "            },\n"
               "            \"room\": \"none\",\n"
               "            \"securityModes\": [],\n"
               "            \"status\": \"notDetected\",\n"
               "            \"supportedAddingModes\": [\n"
               "                \"s2\",\n"
               "                \"smart\"\n"
               "            ],\n"
               "            \"supportedModes\": [\n"
               "                \"accessControl\",\n"
               "                \"authenticated\",\n"
               "                \"unauthenticated\",\n"
               "                \"s0\"\n"
               "            ]\n"
               "        }\n"
               "    ]\n"
               "}",
               answer[ "result" ].dump( 4 ) );

    removeProvisioningEntry( "1490156180" );
    removeProvisioningEntry( "1490156181" );
}

TEST_F( ProvisioningListTests, ZwaveGetProvisioningInfoGetExisting /* 5.1 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.extensions.plugin.run\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"script\": \"HUB:zwave_test/scripts/provisioning\",\n"
                               "        \"scriptParams\": {\n"
                               "            \"command\": \"get_provisioning_info\",\n"
                               "            \"node_id\": 0\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    checkBroadcasts( { []( const Json& broadcast ) {
                          const std::string expected = "{\n"
                                                       "    \"id\": \"ui_broadcast\",\n"
                                                       "    \"msg_subclass\": \"hub.extensions.plugin.ui_broadcast\",\n"
                                                       "    \"result\": {\n"
                                                       "        \"data\": {\n"
                                                       "            \"name\": \"test\",\n"
                                                       "            \"node_id\": 0,\n"
                                                       "            \"room\": \"none\"\n"
                                                       "        },\n"
                                                       "        \"plugin\": \"zwave_test\"\n"
                                                       "    }\n"
                                                       "}";
                          Json broadcastTmp = broadcast;
                          broadcastTmp.erase( "msg_id" );
                          return broadcastTmp.dump( 4 ) == expected;
                      },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "started";
                       },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "finished";
                       } },
                     __LINE__ );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ZwaveGetProvisioningInfoGetMissing /* 5.2 */ ) {
    addDefaultProvisionEntry();

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.extensions.plugin.run\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"script\": \"HUB:zwave_test/scripts/provisioning\",\n"
                               "        \"scriptParams\": {\n"
                               "            \"command\": \"get_provisioning_info\",\n"
                               "            \"node_id\": 42\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    checkBroadcasts( { []( const Json& broadcast ) {
                          const std::string expected = "{\n"
                                                       "    \"id\": \"ui_broadcast\",\n"
                                                       "    \"msg_subclass\": \"hub.extensions.plugin.ui_broadcast\",\n"
                                                       "    \"result\": {\n"
                                                       "        \"data\": {\n"
                                                       "            \"error\": \"fail to get provision info for node "
                                                       "42: Provisioning info not found for node "
                                                       "with id: 42\"\n"
                                                       "        },\n"
                                                       "        \"plugin\": \"zwave_test\"\n"
                                                       "    }\n"
                                                       "}";
                          Json broadcastTmp = broadcast;
                          broadcastTmp.erase( "msg_id" );
                          return broadcastTmp.dump( 4 ) == expected;
                      },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "started";
                       },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "finished";
                       } },
                     __LINE__ );

    removeProvisioningEntry();
}

TEST_F( ProvisioningListTests, ZwaveSetProvisioningInfoSetExisting /* 6.1 */ ) {
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34339 } );

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.extensions.plugin.run\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"script\": \"HUB:zwave_test/scripts/provisioning\",\n"
                               "        \"scriptParams\": {\n"
                               "            \"command\": \"set_provisioning_info\",\n"
                               "            \"node_id\": 0,\n"
                               "            \"name\": \"teapot\",\n"
                               "            \"room\": \"bathroom\"\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    checkBroadcasts( { []( const Json& broadcast ) {
                          const std::string expected =
                              "{\n"
                              "    \"id\": \"ui_broadcast\",\n"
                              "    \"msg_subclass\": \"hub.addons.zwave.nodes.provisioning.updated\",\n"
                              "    \"result\": {\n"
                              "        \"provisioning\": [\n"
                              "            {\n"
                              "                \"_id\": \"1490156180\",\n"
                              "                \"name\": \"teapot\",\n"
                              "                \"room\": \"bathroom\"\n"
                              "            }\n"
                              "        ]\n"
                              "    }\n"
                              "}";
                          Json broadcastTmp = broadcast;
                          broadcastTmp.erase( "msg_id" );
                          return broadcastTmp.dump( 4 ) == expected;
                      },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "started";
                       },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "finished";
                       } },
                     __LINE__ );

    removeProvisioningEntry( "1490156180" );
}

TEST_F( ProvisioningListTests, ZwaveSetProvisioningInfoSetMissing /* 6.2 */ ) {
    addDefaultProvisionEntry( { 58975, 37056, 29754, 11588, 18902, 12511, 34338, 34339 } );

    Json request = PARSE_JSON( "{\n"
                               "    \"method\": \"hub.extensions.plugin.run\",\n"
                               "    \"id\": \"_ID_\",\n"
                               "    \"params\": {\n"
                               "        \"script\": \"HUB:zwave_test/scripts/provisioning\",\n"
                               "        \"scriptParams\": {\n"
                               "            \"command\": \"set_provisioning_info\",\n"
                               "            \"node_id\": 42,\n"
                               "            \"name\": \"teapot\",\n"
                               "            \"room\": \"bathroom\"\n"
                               "        }\n"
                               "    }\n"
                               "}" );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    EXPECT_TRUE( answer[ "error" ].is_null() ) << answer[ "error" ].dump( 4 );

    checkBroadcasts( { []( const Json& broadcast ) {
                          const std::string expected = "{\n"
                                                       "    \"id\": \"ui_broadcast\",\n"
                                                       "    \"msg_subclass\": \"hub.extensions.plugin.ui_broadcast\",\n"
                                                       "    \"result\": {\n"
                                                       "        \"data\": {\n"
                                                       "            \"error\": \"fail to set provision info for node "
                                                       "42: Provisioning info not found for node "
                                                       "with id: 42\"\n"
                                                       "        },\n"
                                                       "        \"plugin\": \"zwave_test\"\n"
                                                       "    }\n"
                                                       "}";
                          Json broadcastTmp = broadcast;
                          broadcastTmp.erase( "msg_id" );
                          return broadcastTmp.dump( 4 ) == expected;
                      },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "started";
                       },
                       []( const Json& broadcast ) {
                           return json_get_str( broadcast, "msg_subclass" ) == "hub.extensions.plugin.run.progress" &&
                                  json_get_str( broadcast[ "result" ], "status" ) == "finished";
                       } },
                     __LINE__ );

    removeProvisioningEntry( "1490156180" );
}

}  // namespace Tests
}  // namespace HubPlatform
