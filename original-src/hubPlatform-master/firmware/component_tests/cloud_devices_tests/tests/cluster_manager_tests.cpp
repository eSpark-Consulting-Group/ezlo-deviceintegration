#include "cloud_devices_tests/tests/fixtures/cluster_manager_fixture.h"

#include "system_utils/environment.h"
#include "common_utils/hub_info_helper.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. Establish connection
DONE        1.1 Different events order
DONE            1.1.1 Certificates, credentials, addresses, registration
DONE            1.1.2 Certificates, credentials, registration, addresses
DONE            1.1.3 Registration, certificates, credentials, addresses
DONE            1.1.4 Registration, addresses, certificates, credentials
DONE        1.2 First credentials are incorrect
DONE        1.3 First address is incorrect
DONE        1.4 Incorrect certificates
DONE            1.4.1 Too old
DONE            1.4.2 Too new
DONE            1.4.3 Wrong UUID
DONE            1.4.4 Wrong serial
DONE            1.4.5 Wrong root
DONE            1.4.6 Root too old
DONE            1.4.7 Is root
DONE            1.4.8 Chain is too long
DONE            1.4.9 Non-secure server
TODO        1.5 Extensions manager reasks subscribed entities
DONE    2. Reset connection
TODO        2.1 Loose certificates
DONE        2.2 Loose credentials
DONE        2.3 Loose addresses
TODO    3. Reestablish connection
DONE    4. Communication through established connection
DONE        4.1 Send request, receive response
DONE        4.2 Receive request, send response
DONE        4.3 Send broadcast
DONE        4.4 Receive broadcast
TODO        4.5 Send request, receive async response
DONE    5. Receive connection
DONE        5.1 Successful login
DONE        5.2 Login with incorrect credentials
TODO        5.3 Incorrect certificates
TODO        5.4 Extensions manager reasks subscribed entities
DONE    6. Communication through received connection
DONE        6.1 Send request, receive response
DONE        6.2 Receive request, send response
DONE        6.3 Send broadcast
DONE        6.4 Receive broadcast
TODO        6.5 Send request, receive async response
DONE    7. Establish and receive connections
DONE        7.1 Do not establish connection to controller that is already connected
DONE        7.2 Do not accept connection from controller we are already connected to
DONE        7.3 Reconnect to controller if it disconnected
DONE        7.4 Resolve two-way connection
DONE            7.4.1 Drop connection to controller with higher serial
DONE            7.4.2 Keep connection to controller with lower serial
TODO    8. Communication through established and received connection
DONE    9. Direct and cloud connection
DONE        9.1 Ignore broadcasts from cloud if we have direct connection

 */

TEST_F( ClusterManagerTest, EstablishConnectionCertificatesCredentialsAddressesRegistration ) {  // 1.1.1

    // Setup
    server().start();

    // Test
    setCredentials( 101110, getSerial(), getUUID(), { "test-1.1.1-key-1", "test-1.1.1-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    icc().registerControllerConnection( getSerial() );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.1.1-key-1" );
        server().sendResponse( request );
    }

    icc().unregisterControllerConnection( getSerial() );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( false );

    // Teardown
    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 101111, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, EstablishConnectionCertificatesCredentialsRegistrationAddresses ) {  // 1.1.2

    // Setup
    server().start();

    // Test
    setCredentials( 101120, getSerial(), getUUID(), { "test-1.1.2-key-1", "test-1.1.2-key-2" } );

    icc().registerControllerConnection( getSerial() );

    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Teardown
    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.1.2-key-1" );
        server().sendResponse( request );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 101121, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionRegistrationCertificatesCredentialsAddresses ) {  // 1.1.3

    // Setup
    server().start();

    // Test
    icc().registerControllerConnection( getSerial() );

    setCredentials( 101130, getSerial(), getUUID(), { "test-1.1.3-key-1", "test-1.1.3-key-2" } );

    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Teardown
    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.1.3-key-1" );
        server().sendResponse( request );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 101131, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionRegistrationAddressesCertificatesCredentials ) {  // 1.1.4

    // Setup
    server().start();

    // Test
    icc().registerControllerConnection( getSerial() );

    setCredentials( 101140, getSerial(), getUUID(), { "test-1.1.4-key-1", "test-1.1.4-key-2" } );

    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Teardown
    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.1.4-key-1" );
        server().sendResponse( request );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 101141, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionFirstCredentialsAreIncorrect ) {  // 1.2

    // Setup
    server().start();

    setCredentials( 101200, getSerial(), getUUID(), { "test-1.2-key-1", "test-1.2-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.2-key-1" );
        server().sendResponse( request,
                               JSON_EMPTY_OBJ,
                               JSON( "code" << -32500 << "data"
                                            << "user.login.badpassword"
                                            << "message"
                                            << "Bad password" ) );
    }

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.2-key-2" );
        server().sendResponse( request );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 101201, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionFirstAddressIsIncorrect ) {  // 1.3

    // Setup
    server().start();

    setCredentials( 101300, getSerial(), getUUID(), { "test-1.3-key-1", "test-1.3-key-2" } );
    setBridgeControllerConnection( getUUID(), { "192.0.2.1:65535", server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Teardown
    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.offline.login.hub" );
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), "test-1.3-key-1" );
        server().sendResponse( request );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 101301, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesTooOld ) {  // 1.4.1

    // Setup
    server().start( "test_controller_too_old.crt" );

    setCredentials( 101410, getSerial(), getUUID(), { "test-1.4.1-key-1", "test-1.4.1-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101411, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesTooNew ) {  // 1.4.2

    // Setup
    server().start( "test_controller_too_new.crt" );

    setCredentials( 101420, getSerial(), getUUID(), { "test-1.4.2-key-1", "test-1.4.2-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101421, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesWrongUUID ) {  // 1.4.3

    // Setup
    server().start( "test_controller_wrong_uuid.crt" );

    setCredentials( 101430, getSerial(), getUUID(), { "test-1.4.3-key-1", "test-1.4.3-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101431, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesWrongSerial ) {  // 1.4.4

    // Setup
    server().start( "test_controller_higher_serial.crt" );

    setCredentials( 101440, getSerial(), getUUID(), { "test-1.4.4-key-1", "test-1.4.4-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    // Current behavior (Check of common name is temporary disabled):
    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Expected behavior:
    // std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    // EXPECT_FALSE( server().isConnected() );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101441, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesWrongRoot ) {  // 1.4.5

    // Setup
    server().start( "test_controller_root_wrong.crt", "", "root_wrong.crt" );

    setCredentials( 101450, getSerial(), getUUID(), { "test-1.4.5-key-1", "test-1.4.5-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101451, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesRootTooOld ) {  // 1.4.6

    // Setup
    server().start();

    setCredentials(
        101460, getSerial(), getUUID(), { "test-1.4.6-key-1", "test-1.4.6-key-2" }, FwCertsType::ExpiredRoot );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101461, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesIsRoot ) {  // 1.4.7

    // Setup
    server().start( "test_controller_is_root.crt", "test_controller_is_root.key", "test_controller_is_root.crt" );

    setCredentials( 101470, getSerial(), getUUID(), { "test-1.4.7-key-1", "test-1.4.7-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101471, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesChainTooLong ) {  // 1.4.8

    // Setup
    server().start( "test_controller_deep.crt", "test_controller_deep.key" );

    setCredentials( 101480, getSerial(), getUUID(), { "test-1.4.8-key-1", "test-1.4.8-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101481, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, EstablishConnectionIncorrectCertificatesNonSecureServer ) {  // 1.4.9

    // Setup
    server().start( "", "", "" );

    setCredentials( 101490, getSerial(), getUUID(), { "test-1.4.9-key-1", "test-1.4.9-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Test
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 101491, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, ResetConnectionLooseCertificates ) {  // 2.2

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 102200, getSerial(), getUUID(), { "test-2.2-key-1", "test-2.2-key-2" } );

    // Test
    setCredentials( 102201, "", "", {}, FwCertsType::Correct );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( false );

    // Teardown
    icc().unregisterControllerConnection( getSerial() );

    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 102202, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, ResetConnectionLooseAddresses ) {  // 2.3

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 102300, getSerial(), getUUID(), { "test-2.3-key-1", "test-2.3-key-2" } );

    // Test
    setBridgeControllerConnection( getUUID(), {} );

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( false );

    // Teardown
    icc().unregisterControllerConnection( getSerial() );

    setCredentials( 102301, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, CommunicationThroughEstablishedConnectionSendRequestReceiveResponse ) {  // 4.1

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 104100, getSerial(), getUUID(), { "test-4.1-key-1", "test-4.1-key-2" } );

    // Test
    {
        auto message = JSON( "method"
                             << "hub.bridge.connections.list" );
        message[ "params" ] = JSON_EMPTY_OBJ;
        auto response = TEST_CONTROLLER_SERVER_SEND_TO_HUB_AND_WAIT_ANSWER( message );
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"" +
                       getUUID() +
                       "\",\n"
                       "            \"ip\": [\n"
                       "                \"" +
                       server().getAddress() +
                       "\"\n"
                       "            ],\n"
                       "            \"serial\": \"" +
                       getSerial() +
                       "\",\n"
                       "            \"status\": \"direct_connection\"\n"
                       "        }\n"
                       "    ]\n"
                       "}",
                   response[ "result" ].dump( 4 ) );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 104101, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, CommunicationThroughEstablishedConnectionReceiveRequestSendResponse ) {  // 4.2

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 104200, getSerial(), getUUID(), { "test-4.2-key-1", "test-4.2-key-2" } );

    std::mutex responseMutex;
    std::condition_variable responseCondVar;
    Json actualResponse;

    {
        auto message = JSON( "method"
                             << "hub.info.get" );
        message[ "params" ] = JSON( "_id"
                                    << "some_item_id"
                                    << "value" << 30 );
        icc().sendAsyncRequest( getSerial(), message, [&]( const JsonObject& response ) {
            actualResponse = response;
            responseCondVar.notify_one();
        } );
    }

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.info.get" );
        EXPECT_EQ( json_get_str( request[ "params" ], "_id" ), "some_item_id" );
        EXPECT_EQ( json_get_int( request[ "params" ], "value" ), 30 );
        server().sendResponse( request,
                               JSON( "testKey"
                                     << "testValue" ) );
    }

    std::unique_lock< std::mutex > responseLock( responseMutex );
    EXPECT_EQ( responseCondVar.wait_for( responseLock, std::chrono::milliseconds( 500 ) ), std::cv_status::no_timeout );
    EXPECT_EQ( "{\n"
               "    \"testKey\": \"testValue\"\n"
               "}",
               actualResponse[ "result" ].dump( 4 ) );

    TEST_UNREGISTER_AND_TEARDOWN( 104201, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, CommunicationThroughEstablishedConnectionSendBroadcast ) {  // 4.3

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 104300, getSerial(), getUUID(), { "test-4.3-key-1", "test-4.3-key-2" } );

    externalControllers().subscribeOnItemChanges( getSerial(), { "item-1.1.3" } );

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "hub.items.list" );
        server().sendResponse( request,
                               PARSE_JSON( "{\n"
                                           "    \"items\": [" +
                                           generateItem( "item-1.1.3" ) +
                                           "    ]\n"
                                           "}" ) );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( getSerial(), "item-1.1.3" ), broadcast.data.dump( 4 ) );
    }

    server().sendBroadcast( "hub.item.updated",
                            PARSE_JSON( "{\n"
                                        "    \"_id\": \"item-1.1.3\",\n"
                                        "    \"deviceCategory\": \"dimmable_light\",\n"
                                        "    \"deviceId\": \"5e903a57e4ed9016766749ab\",\n"
                                        "    \"deviceName\": \"Dimmer\",\n"
                                        "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                                        "    \"name\": \"switch\",\n"
                                        "    \"notifications\": null,\n"
                                        "    \"roomName\": \"\",\n"
                                        "    \"serviceNotification\": false,\n"
                                        "    \"syncNotification\": false,\n"
                                        "    \"userNotification\": true,\n"
                                        "    \"value\": true,\n"
                                        "    \"valueFormatted\": \"true\",\n"
                                        "    \"valueType\": \"bool\"\n"
                                        "}" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.3\",\n"
                   "    \"controllerId\": \"" +
                       getSerial() +
                       "\",\n"
                       "    \"deviceId\": \"device-1.1\",\n"
                       "    \"name\": \"switch\",\n"
                       "    \"value\": true,\n"
                       "    \"valueFormatted\": \"true\",\n"
                       "    \"valueType\": \"bool\"\n"
                       "}",
                   broadcast.data.dump( 4 ) );
    }

    externalControllers().unsubscribeFromItemChanges( getSerial(), { "item-1.1.3" } );
    TEST_UNREGISTER_AND_TEARDOWN( 104301, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, CommunicationThroughEstablishedConnectionReceiveBroadcast ) {  // 4.4

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 104400, getSerial(), getUUID(), { "test-4.4-key-1", "test-4.4-key-2" } );

    std::string broadcastBody = "{\n"
                                "    \"_id\": \"5e903a58e4ed9016766749b5\",\n"
                                "    \"deviceCategory\": \"dimmable_light\",\n"
                                "    \"deviceId\": \"5e903a57e4ed9016766749ab\",\n"
                                "    \"deviceName\": \"Dimmer\",\n"
                                "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                                "    \"name\": \"switch\",\n"
                                "    \"notifications\": null,\n"
                                "    \"roomName\": \"\",\n"
                                "    \"serviceNotification\": false,\n"
                                "    \"syncNotification\": false,\n"
                                "    \"userNotification\": true,\n"
                                "    \"value\": true,\n"
                                "    \"valueFormatted\": \"true\",\n"
                                "    \"valueType\": \"bool\"\n"
                                "}";

    {
        auto message = PARSE_JSON( broadcastBody );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        nma().sendBroadcast( HUB_ITEM_UPDATED, message );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );

        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
        EXPECT_EQ( broadcastBody, broadcast[ "result" ].dump( 4 ) );
    }

    {
        auto broadcast = TEST_CONTROLLER_SERVER_WAIT_FOR_BROADCAST( "hub.item.updated" );

        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
        EXPECT_EQ( broadcastBody, broadcast[ "result" ].dump( 4 ) );
    }

    TEST_UNREGISTER_AND_TEARDOWN( 104401, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, ReceiveConnectionSuccessfulLogin ) {  // 5.1

    // Setup
    setCredentials( 105100, getSerial(), getUUID(), { "test-5.1-key-1", "test-5.1-key-2" } );

    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    // Test
    EXPECT_TRUE( client().start( localServerPort, getUUID(), "test-5.1-key-1" ) );

    // Teardown
    setCredentials( 105101, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, ReceiveConnectionLoginWithIncorrectCredentials ) {  // 5.2

    // Setup
    setCredentials( 105200, getSerial(), getUUID(), { "test-5.2-key-1", "test-5.2-key-2" } );

    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    // Test
    EXPECT_FALSE( client().start( localServerPort, getUUID(), "test-5.2-wrong-key" ) );

    // Teardown
    setCredentials( 105201, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, CommunicationThroughReceivedConnectionSendRequestReceiveResponse ) {  // 6.1

    TEST_SETUP_AND_LOGIN( 106100, getSerial(), getUUID(), { "test-6.1-key-1", "test-6.1-key-2" } );

    // Test
    {
        auto message = JSON( "method"
                             << "hub.bridge.connections.list" );
        message[ "params" ] = JSON_EMPTY_OBJ;
        auto response = TEST_CONTROLLER_CLIENT_SEND_TO_HUB_AND_WAIT_ANSWER( message );
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"" +
                       getUUID() +
                       "\",\n"
                       "            \"ip\": [],\n"
                       "            \"serial\": \"" +
                       getSerial() +
                       "\",\n"
                       "            \"status\": \"direct_connection\"\n"
                       "        }\n"
                       "    ]\n"
                       "}",
                   response[ "result" ].dump( 4 ) );
    }

    // Teardown
    setCredentials( 106101, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, CommunicationThroughReceivedConnectionReceiveRequestSendResponse ) {  // 6.2

    TEST_SETUP_AND_LOGIN( 106200, getSerial(), getUUID(), { "test-6.2-key-1", "test-6.2-key-2" } );

    std::mutex responseMutex;
    std::condition_variable responseCondVar;
    Json actualResponse;

    {
        auto message = JSON( "method"
                             << "hub.info.get" );
        message[ "params" ] = JSON( "_id"
                                    << "some_item_id"
                                    << "value" << 30 );
        icc().sendAsyncRequest( getSerial(), message, [&]( const JsonObject& response ) {
            actualResponse = response;
            responseCondVar.notify_one();
        } );
    }

    {
        auto request = TEST_CONTROLLER_CLIENT_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "hub.info.get" );
        EXPECT_EQ( json_get_str( request[ "params" ], "_id" ), "some_item_id" );
        EXPECT_EQ( json_get_int( request[ "params" ], "value" ), 30 );
        client().sendResponse( request,
                               JSON( "testKey"
                                     << "testValue" ) );
    }

    std::unique_lock< std::mutex > responseLock( responseMutex );
    EXPECT_EQ( responseCondVar.wait_for( responseLock, std::chrono::seconds( 5 ) ), std::cv_status::no_timeout );
    EXPECT_EQ( "{\n"
               "    \"testKey\": \"testValue\"\n"
               "}",
               actualResponse[ "result" ].dump( 4 ) );

    // Teardown
    setCredentials( 106201, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, CommunicationThroughReceivedConnectionSendBroadcast ) {  // 6.3

    TEST_SETUP_AND_LOGIN( 106300, getSerial(), getUUID(), { "test-6.3-key-1", "test-6.3-key-2" } );

    externalControllers().subscribeOnItemChanges( getSerial(), { "item-1.1.3" } );

    {
        auto request = TEST_CONTROLLER_CLIENT_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "hub.items.list" );
        client().sendResponse( request,
                               PARSE_JSON( "{\n"
                                           "    \"items\": [" +
                                           generateItem( "item-1.1.3" ) +
                                           "    ]\n"
                                           "}" ) );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( getSerial(), "item-1.1.3" ), broadcast.data.dump( 4 ) );
    }

    client().sendBroadcast( "hub.item.updated",
                            PARSE_JSON( "{\n"
                                        "    \"_id\": \"item-1.1.3\",\n"
                                        "    \"deviceCategory\": \"dimmable_light\",\n"
                                        "    \"deviceId\": \"5e903a57e4ed9016766749ab\",\n"
                                        "    \"deviceName\": \"Dimmer\",\n"
                                        "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                                        "    \"name\": \"switch\",\n"
                                        "    \"notifications\": null,\n"
                                        "    \"roomName\": \"\",\n"
                                        "    \"serviceNotification\": false,\n"
                                        "    \"syncNotification\": false,\n"
                                        "    \"userNotification\": true,\n"
                                        "    \"value\": true,\n"
                                        "    \"valueFormatted\": \"true\",\n"
                                        "    \"valueType\": \"bool\"\n"
                                        "}" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.3\",\n"
                   "    \"controllerId\": \"" +
                       getSerial() +
                       "\",\n"
                       "    \"deviceId\": \"device-1.1\",\n"
                       "    \"name\": \"switch\",\n"
                       "    \"value\": true,\n"
                       "    \"valueFormatted\": \"true\",\n"
                       "    \"valueType\": \"bool\"\n"
                       "}",
                   broadcast.data.dump( 4 ) );
    }

    // Teardown
    externalControllers().unsubscribeFromItemChanges( getSerial(), { "item-1.1.3" } );
    setCredentials( 106301, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, CommunicationThroughReceivedConnectionReceiveBroadcast ) {  // 6.4

    TEST_SETUP_AND_LOGIN( 106400, getSerial(), getUUID(), { "test-6.4-key-1", "test-6.4-key-2" } );

    std::string broadcastBody = "{\n"
                                "    \"_id\": \"5e903a58e4ed9016766749b5\",\n"
                                "    \"deviceCategory\": \"dimmable_light\",\n"
                                "    \"deviceId\": \"5e903a57e4ed9016766749ab\",\n"
                                "    \"deviceName\": \"Dimmer\",\n"
                                "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                                "    \"name\": \"switch\",\n"
                                "    \"notifications\": null,\n"
                                "    \"roomName\": \"\",\n"
                                "    \"serviceNotification\": false,\n"
                                "    \"syncNotification\": false,\n"
                                "    \"userNotification\": true,\n"
                                "    \"value\": true,\n"
                                "    \"valueFormatted\": \"true\",\n"
                                "    \"valueType\": \"bool\"\n"
                                "}";

    {
        auto message = PARSE_JSON( broadcastBody );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        nma().sendBroadcast( HUB_ITEM_UPDATED, message );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );

        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
        EXPECT_EQ( broadcastBody, broadcast[ "result" ].dump( 4 ) );
    }

    {
        auto broadcast = TEST_CONTROLLER_CLIENT_WAIT_FOR_BROADCAST( "hub.item.updated" );

        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
        EXPECT_EQ( broadcastBody, broadcast[ "result" ].dump( 4 ) );
    }

    // Teardown
    setCredentials( 106401, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, DoNotEstablishConnectionToControllerThatIsAlreadyConnected ) {  // 7.1

    // Setup (connect to firmware)
    TEST_SETUP_AND_LOGIN( 107100, getSerial(), getUUID(), { "test-7.1-key-1", "test-7.1-key-2" } );

    // Test (configure firmware for establishing connection)
    server().start();
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );
    icc().registerControllerConnection( getSerial() );

    expectServerNotConnected( -1, __FILE__, __LINE__ );

    // Teardown
    icc().unregisterControllerConnection( getSerial() );
    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 107101, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, DoNotAcceptConnectionFromControllerWeAreAlreadyConnectedTo ) {  // 7.2

    // Setup (configure firmware for establishing connection)
    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 107200, getSerial(), getUUID(), { "test-7.2-key-1", "test-7.2-key-2" } );

    // Test (connect to firmware)
    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    EXPECT_FALSE( client().start( localServerPort, getUUID(), "test-7.2-key-1" ) );

    // Teardown
    TEST_UNREGISTER_AND_TEARDOWN( 107201, getSerial(), getUUID() );
}

TEST_F( ClusterManagerTest, ReconnectToControllerIfItDisconnected ) {  // 7.3

    // Setup (connect to firmware)
    TEST_SETUP_AND_LOGIN( 107300, getSerial(), getUUID(), { "test-7.3-key-1", "test-7.3-key-2" } );

    // Test (configure firmware for establishing connection)
    server().start();
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );
    icc().registerControllerConnection( getSerial() );

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    client().stop();

    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Teardown
    icc().unregisterControllerConnection( getSerial() );
    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 107301, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, ResolveTwoWayConnectionDropConnectionToControllerWithHigherSerial ) {  // 7.4.1

    // Ensure this test have sense
    ASSERT_LE( HubInfoHelper::getSerial(), getHigherSerial() );

    // Setup: Start test server
    server().start( "test_controller_higher_serial.crt" );

    // Setup: Configure firmware for both connecting to test server and expect connection from test client
    setCredentials( 107410, getHigherSerial(), getUUID(), { "test-7.4.1-key-1", "test-7.4.1-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Setup: Wait for firmware to start local server
    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    // Setup: Ask firmware to connect to test server and wait for connection
    icc().registerControllerConnection( getHigherSerial() );
    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Setup: Wait for login request from firmware
    auto loginToTestServerRequest = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();
    EXPECT_EQ( json_get_str( loginToTestServerRequest, "method" ), "hub.offline.login.hub" );
    EXPECT_EQ( json_get_str( loginToTestServerRequest[ "params" ], "hub" ), getLocalControllerUuid() );
    EXPECT_EQ( json_get_str( loginToTestServerRequest[ "params" ], "token" ), "test-7.4.1-key-1" );

    // Setup: While firmware waits for login response, connect to firmware with test client
    // Note: both test server and test client share same serial and uuid
    EXPECT_TRUE( client().start( localServerPort, getUUID(), "test-7.4.1-key-1" ) );

    // Setup: Now send response to previously received login request
    server().sendResponse( loginToTestServerRequest );

    // Test: Expect firmware to drop its connection as it has lower serial
    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( false );

    // Teardown
    icc().unregisterControllerConnection( getHigherSerial() );

    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 107411, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, ResolveTwoWayConnectionKeepConnectionToControllerWithLowerSerial ) {  // 7.4.2

    // Ensure this test have sense
    ASSERT_GT( HubInfoHelper::getSerial(), getLowerSerial() );

    // Setup: Start test server
    server().start( "test_controller_lower_serial.crt" );

    // Setup: Configure firmware for both connecting to test server and expect connection from test client
    setCredentials( 107420, getLowerSerial(), getUUID(), { "test-7.4.2-key-1", "test-7.4.2-key-2" } );
    setBridgeControllerConnection( getUUID(), { server().getAddress() } );

    // Setup: Wait for firmware to start local server
    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    // Setup: Ask firmware to connect to test server and wait for connection
    icc().registerControllerConnection( getLowerSerial() );
    TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( true );

    // Setup: Wait for login request from firmware
    auto loginToTestServerRequest = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();
    EXPECT_EQ( json_get_str( loginToTestServerRequest, "method" ), "hub.offline.login.hub" );
    EXPECT_EQ( json_get_str( loginToTestServerRequest[ "params" ], "hub" ), getLocalControllerUuid() );
    EXPECT_EQ( json_get_str( loginToTestServerRequest[ "params" ], "token" ), "test-7.4.2-key-1" );

    // Setup: While firmware waits for login response, connect to firmware with test client
    // Note: both test server and test client share same serial and uuid
    EXPECT_TRUE( client().start( localServerPort, getUUID(), "test-7.4.2-key-1", "test_controller_lower_serial.crt" ) );

    // Setup: Now send response to previously received login request
    server().sendResponse( loginToTestServerRequest );

    // Test: Expect firmware not to drop its connection as it has higher serial
    std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    EXPECT_TRUE( server().isConnected() );

    // Teardown
    icc().unregisterControllerConnection( getLowerSerial() );

    setBridgeControllerConnection( getUUID(), {} );
    setCredentials( 107421, "", "", {}, FwCertsType::Broken );
}

TEST_F( ClusterManagerTest, IgnoreBroadcastsFromCloudIfWeHaveDirectConnection ) {  // 9.1

    TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( 104300, getSerial(), getUUID(), { "test-9.1-key-1", "test-9.1-key-2" } );

    externalControllers().subscribeOnItemChanges( getSerial(), { "item-1.1.3" } );

    {
        auto request = TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "hub.items.list" );
        server().sendResponse( request,
                               PARSE_JSON( "{\n"
                                           "    \"items\": [" +
                                           generateItem( "item-1.1.3" ) +
                                           "    ]\n"
                                           "}" ) );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, LOCAL_SERVER_STARTED );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( getSerial(), "item-1.1.3" ), broadcast.data.dump( 4 ) );
    }

    sendBroadcast( "hub.item.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.3\",\n"
                               "    \"deviceCategory\": \"dimmable_light\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Dimmer\",\n"
                               "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                               "    \"name\": \"switch\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"value\": true,\n"
                               "    \"valueFormatted\": \"true\",\n"
                               "    \"valueType\": \"bool\",\n"
                               "    \"nma_timestamp\": 1586762888663,\n"
                               "    \"controller_id\": \"" +
                               getSerial() +
                               "\",\n"
                               "    \"controller_uuid\": \"" +
                               getUUID() +
                               "\""
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid" << getUUID() ) );

    NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS();

    externalControllers().unsubscribeFromItemChanges( getSerial(), { "item-1.1.3" } );
    TEST_UNREGISTER_AND_TEARDOWN( 104301, getSerial(), getUUID() );
}

}  // namespace Tests
}  // namespace HubPlatform
