#include "cloud_devices_tests/tests/fixtures/external_controllers_fixture.h"

#include "system_utils/environment.h"

namespace HubPlatform {
namespace Tests {

std::string ExternalControllersTest::ms_firmwareCertificate;
std::string ExternalControllersTest::ms_firmwareKey;
std::string ExternalControllersTest::ms_firmwareParent;
std::string ExternalControllersTest::ms_firmwareExpiredParent;

ExternalControllersTest::ExternalControllersTest()
: m_externalControllers( nma() ) {
    if( ms_firmwareCertificate.empty() ) {
        std::ifstream crtStream( Environment::getFirmwareRootFolder() + "/certs/test_firmware.crt" );
        ms_firmwareCertificate = { std::istreambuf_iterator< char >( crtStream ), {} };
        std::ifstream keyStream( Environment::getFirmwareRootFolder() + "/certs/test_firmware.key" );
        ms_firmwareKey = { std::istreambuf_iterator< char >( keyStream ), {} };
        std::ifstream parentStream( Environment::getFirmwareRootFolder() + "/certs/root.crt" );
        ms_firmwareParent = { std::istreambuf_iterator< char >( parentStream ), {} };
        std::ifstream expiredParentStream( Environment::getFirmwareRootFolder() + "/certs/root_too_old.crt" );
        ms_firmwareExpiredParent = { std::istreambuf_iterator< char >( expiredParentStream ), {} };
    }
}

ExternalControllers::RemoteInterface& ExternalControllersTest::externalControllers() {
    return m_externalControllers;
}

std::vector< std::string > ExternalControllersTest::dump( const std::vector< JsonObject >& jsons ) {
    std::vector< std::string > result;
    std::transform(
        jsons.begin(), jsons.end(), std::back_inserter( result ), []( const Json& json ) { return json.dump( 4 ); } );
    std::sort( result.begin(), result.end() );
    return result;
}

std::string ExternalControllersTest::generateItem( const std::string& itemId ) {
    return generateItem( "", itemId );
}

std::string ExternalControllersTest::generateItem( const std::string& controllerId, const std::string& itemId ) {
    if( itemId == "item-1.1.1" ) {
        return "{\n"
               "    \"_id\": \"item-1.1.1\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"elementType\": \"userCode\",\n"
               "    \"elementsMaxNumber\": 100,\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"user_codes\",\n"
               "    \"show\": true,\n"
               "    \"userCodeRestriction\": \"^\\\\d{4}$\",\n"
               "    \"value\": {\n"
               "        \"1\": {\n"
               "            \"code\": \"1111\",\n"
               "            \"name\": \"test1\"\n"
               "        },\n"
               "        \"2\": {\n"
               "            \"code\": \"2222\",\n"
               "            \"name\": \"test2\"\n"
               "        }\n"
               "    },\n"
               "    \"valueFormatted\": \"\",\n"
               "    \"valueType\": \"dictionary\"\n"
               "}";
    }
    else if( itemId == "item-1.1.2" ) {
        return "{\n"
               "    \"_id\": \"item-1.1.2\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"hasGetter\": false,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"dimmer_up\",\n"
               "    \"show\": true,\n"
               "    \"value\": 0,\n"
               "    \"valueFormatted\": \"0\",\n"
               "    \"valueType\": \"int\"\n"
               "}";
        ;
    }
    else if( itemId == "item-1.1.3" ) {
        return "{\n"
               "    \"_id\": \"item-1.1.3\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"switch\",\n"
               "    \"show\": true,\n"
               "    \"value\": false,\n"
               "    \"valueFormatted\": \"false\",\n"
               "    \"valueType\": \"bool\"\n"
               "}";
    }
    throw std::runtime_error( "Please specify test item id from supported list" );
}

JsonArray ExternalControllersTest::generateItems( const std::string& controllerId,
                                                  const std::vector< std::string >& itemIds ) {
    auto result = Json::array();
    for( auto& itemId : itemIds )
        result.push_back( PARSE_JSON( generateItem( controllerId, itemId ) ) );

    return result;
}

std::string ExternalControllersTest::generateDevice( const std::string& deviceId ) {
    return generateDevice( "", deviceId );
}

std::string ExternalControllersTest::generateDevice( const std::string& controllerId, const std::string& deviceId ) {
    if( deviceId == "device-1.1" ) {
        return "{\n"
               "    \"_id\": \"device-1.1\",\n"
               "    \"batteryPowered\": false,\n"
               "    \"category\": \"dimmable_light\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceTypeId\": \"134_3_25\",\n"
               "    \"gatewayId\": \"gateway-1\",\n"
               "    \"info\": {\n"
               "        \"manufacturer\": \"Aeotec\",\n"
               "        \"model\": \"DSC25\"\n"
               "    },\n"
               "    \"name\": \"Dimmer\",\n"
               "    \"parentDeviceId\": \"\",\n"
               "    \"reachable\": true,\n"
               "    \"ready\": true,\n"
               "    \"roomId\": \"\",\n"
               "    \"security\": \"no\",\n"
               "    \"status\": \"idle\",\n"
               "    \"subcategory\": \"dimmable_plugged\",\n"
               "    \"type\": \"dimmer.outlet\"\n"
               "}";
    }
    else if( deviceId == "device-1.2" ) {
        return "{\n"
               "    \"_id\": \"device-1.2\",\n"
               "    \"batteryPowered\": true,\n"
               "    \"category\": \"keypad\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceTypeId\": \"CentraLite_3400\",\n"
               "    \"gatewayId\": \"gateway-2\",\n"
               "    \"info\": {\n"
               "        \"manufacturer\": \"CentraLite\",\n"
               "        \"model\": \"3400\"\n"
               "    },\n"
               "    \"name\": \"Centralite Keypad\",\n"
               "    \"parentDeviceId\": \"\",\n"
               "    \"reachable\": true,\n"
               "    \"ready\": true,\n"
               "    \"roomId\": \"\",\n"
               "    \"security\": \"no\",\n"
               "    \"status\": \"idle\",\n"
               "    \"subcategory\": \"\",\n"
               "    \"type\": \"device\"\n"
               "}";
        ;
    }
    else if( deviceId == "device-1.3" ) {
        return "{\n"
               "    \"_id\": \"device-1.3\",\n"
               "    \"batteryPowered\": true,\n"
               "    \"category\": \"temperature\",\n" +
               ( controllerId.empty() ? "" : "    \"controllerId\": \"" + controllerId + "\",\n" ) +
               "    \"deviceTypeId\": \"CentraLite_3400\",\n"
               "    \"gatewayId\": \"gateway-2\",\n"
               "    \"info\": {\n"
               "        \"manufacturer\": \"CentraLite\",\n"
               "        \"model\": \"3400\"\n"
               "    },\n"
               "    \"name\": \"Temperature Sensor\",\n"
               "    \"parentDeviceId\": \"device-1.2\",\n"
               "    \"reachable\": true,\n"
               "    \"ready\": true,\n"
               "    \"roomId\": \"\",\n"
               "    \"security\": \"no\",\n"
               "    \"status\": \"idle\",\n"
               "    \"subcategory\": \"\",\n"
               "    \"type\": \"sensor\"\n"
               "}";
    }
    throw std::runtime_error( "Please specify test device id from supported list" );
}

JsonArray ExternalControllersTest::generateDevices( const std::string& controllerId,
                                                    const std::vector< std::string >& deviceIds ) {
    auto result = Json::array();
    for( auto& deviceId : deviceIds )
        result.push_back( PARSE_JSON( generateDevice( controllerId, deviceId ) ) );

    return result;
}

Json ExternalControllersTest::makeRawCommandResult( const std::string& id,
                                                    const std::string& method,
                                                    Json innerCommandResult ) {
    Json result = R"(
    {
        "action_used": "instant",
        "controller_response": {
            "api": "1.0",
            "error": null
        }
    })"_json;

    Json& controllerResponse = result[ "controller_response" ];
    controllerResponse[ "id" ] = id;
    controllerResponse[ "method" ] = method;
    controllerResponse[ "result" ] = std::move( innerCommandResult );

    return result;
};

void ExternalControllersTest::subscribeOnItemChanges( const std::string& controllerId,
                                                      const std::vector< std::string >& itemsIds ) {
    // Subscribe on item changes
    externalControllers().subscribeOnItemChanges( controllerId, itemsIds );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_raw_command" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto result = JSON( "items" << generateItems( controllerId, { "item-1.1.1", "item-1.1.2", "item-1.1.3" } ) );
        auto controllerUuid = controllerId + "-uuid";
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.items.list", std::move( result ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid" << controllerUuid ) );
    }

    for( auto& itemId : itemsIds ) {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( controllerId, itemId ), broadcast.data.dump( 4 ) );
    }
}

void ExternalControllersTest::subscribeOnDeviceChanges( const std::string& controllerId,
                                                        const std::vector< std::string >& devicesIds ) {
    // Subscribe on item changes
    externalControllers().subscribeOnDevicesChanges( controllerId, devicesIds );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_raw_command" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto result =
            JSON( "devices" << generateDevices( controllerId, { "device-1.1", "device-1.2", "device-1.3" } ) );
        auto controllerUuid = controllerId + "-uuid";
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.devices.list", std::move( result ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid" << controllerUuid ) );
    }

    for( auto& deviceId : devicesIds ) {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_DEVICE_ADDED );
        EXPECT_EQ( generateDevice( controllerId, deviceId ), broadcast.data.dump( 4 ) );
    }
}

void ExternalControllersTest::setCredentials( int version,
                                              const std::string& controllerId,
                                              const std::string& controllerUuid,
                                              const std::vector< std::string >& keys,
                                              FwCertsType certsType ) {
    auto peers = JSON_EMPTY_OBJ;
    if( !controllerUuid.empty() ) {
        peers[ controllerUuid ] = JSON( "id" << controllerId << "tokens" << keys );
    }

    Json result;
    result[ "version" ] = version;
    result[ "clients" ] = JSON_EMPTY_OBJ;
    result[ "peers" ] = peers;

    switch( certsType ) {
    case FwCertsType::Correct:
        result[ "current" ] = JSON_ARRAY( JSON( "certificate" << ms_firmwareCertificate << "private" << ms_firmwareKey
                                                              << "public_parent" << ms_firmwareParent ) );
        break;
    case FwCertsType::ExpiredRoot:
        result[ "current" ] = JSON_ARRAY( JSON( "certificate" << ms_firmwareCertificate << "private" << ms_firmwareKey
                                                              << "public_parent" << ms_firmwareExpiredParent ) );
        break;
    case FwCertsType::Broken:
        result[ "current" ] = JSON_ARRAY( JSON( "certificate"
                                                << "-----BROKEN CERTIFICATE-----"
                                                << "private"
                                                << "-----BROKEN PRIVATE KEY-----"
                                                << "public_parent"
                                                << "-----BROKEN CERTIFICATE-----" ) );
        break;
    }

    auto message = JSON( "method"
                         << "cloud.access_keys_controller_sync" );
    message[ "result" ] = result;

    LOGGER_ERROR( "message: " + cc::j( message ) );

    nma().assignMessageId( message );
    nma().sendtoHub( message, ComponentTestApplication::ConnectionType::NMA );
}

JsonObject ExternalControllersTest::setBridgeControllerConnection( const std::string& controllerUuid,
                                                                   const std::vector< std::string >& ips ) {
    auto message = JSON( "method"
                         << "hub.bridge.controller.connection.set" );
    message[ "params" ][ "_id" ] = controllerUuid;
    message[ "params" ][ "ip" ] = ips;

    nma().assignMessageId( message );
    return nma().sendtoHubAndGetAnswer( message, ComponentTestApplication::ConnectionType::NMA );
}

JsonObject ExternalControllersTest::getBridgeConnectionsList() {
    auto message = JSON( "method"
                         << "hub.bridge.connections.list" );
    message[ "params" ] = JSON_EMPTY_OBJ;

    nma().assignMessageId( message );
    return nma().sendtoHubAndGetAnswer( message, ComponentTestApplication::ConnectionType::NMA );
}

}  // namespace Tests
}  // namespace HubPlatform
