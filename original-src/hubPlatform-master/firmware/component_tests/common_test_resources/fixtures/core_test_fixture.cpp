#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/scalable_value.h"
#include "cored/model/cored_device_setting.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace Tests {

void CoreNmaTest::testHouseModeAlarmedBroadcast( const std::string& mode,
                                                 const std::string& deviceId,
                                                 int codeLine,
                                                 std::chrono::seconds timeoutSeconds ) {
    const std::string codeLineStr = getCodeLineString( codeLine );

    const Json broadcast = waitForBroadcast( "hub.modes.alarmed", codeLineStr, timeoutSeconds );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), mode ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId ) << codeLineStr;
    int64_t timestamp;
    EXPECT_TRUE( json_get( result, "timestamp", timestamp ) ) << codeLineStr;
}

void CoreNmaTest::testHouseModeAlarmedPhaseStatusBroadcast( const std::string& mode,
                                                            const std::string& deviceId,
                                                            const std::string& phase,
                                                            const std::string& status,
                                                            int codeLine,
                                                            bool silent,
                                                            std::chrono::seconds timeoutSeconds ) {
    const std::string codeLineStr = getCodeLineString( codeLine );

    const Json broadcast = waitForBroadcast( "hub.modes.alarmed", codeLineStr, timeoutSeconds );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), mode ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "phase" ), phase ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "silent" ), silent ) << codeLineStr;
    int64_t timestamp;
    EXPECT_TRUE( json_get( result, "timestamp", timestamp ) ) << codeLineStr;
}

void CoreNmaTest::testHouseModeAlarmedPhaseStatusMainBroadcast( const std::string& mode,
                                                                const std::string& deviceId,
                                                                const std::string& status,
                                                                int codeLine,
                                                                std::chrono::seconds timeoutSeconds ) {
    const std::string codeLineStr = getCodeLineString( codeLine );

    const Json broadcast = waitForBroadcast( "hub.modes.alarmed", codeLineStr, timeoutSeconds );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), mode ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "phase" ), "main" ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
    EXPECT_EQ( result.count( "pendingDelay" ), 0 ) << codeLineStr;
    int64_t timestamp;
    EXPECT_TRUE( json_get( result, "timestamp", timestamp ) ) << codeLineStr;
}

void CoreNmaTest::testHouseModesSwitchedBroadcast( const std::string& mode,
                                                   const std::string& status,
                                                   int codeLine,
                                                   std::chrono::seconds timeoutSeconds ) {
    const std::string codeLineStr = getCodeLineString( codeLine );

    JsonObject broadcast =
        waitForBroadcast( "hub.modes.switched",
                          "testHouseModesSwitchedBroadcast mode: " + mode + ", status: " + status + ", " + codeLineStr,
                          timeoutSeconds );
    testHouseModesSwitchedBroadcast( broadcast, mode, status, codeLine );
}

void CoreNmaTest::testHouseModesSwitchedBroadcast( const JsonObject& broadcast,
                                                   const std::string& mode,
                                                   const std::string& status,
                                                   int codeLine ) {
    const std::string codeLineStr = getCodeLineString( codeLine );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.switched" ) << codeLineStr;
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "to" ), mode ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
    int64_t timestamp;
    EXPECT_TRUE( json_get( result, "timestamp", timestamp ) ) << codeLineStr;
}

CoreDRemoteInterface::CoreDInterface::DeviceID CoreNmaTest::addDevice(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& name,
    const std::string& type,
    const std::string& deviceTypeId,
    const std::string& category,
    const std::string& subcategory,
    bool batteryPowered,
    int codeLine ) {
    CoreD::Model::Device device =
        createDevice( coreRemoteInterface, name, type, deviceTypeId, category, subcategory, batteryPowered, codeLine );
    return addDevice( coreRemoteInterface, device, codeLine );
}

CoreDRemoteInterface::CoreDInterface::DeviceID CoreNmaTest::addParentDeviceWithRoom(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& name,
    const std::string& type,
    const std::string& deviceTypeId,
    const std::string& category,
    const std::string& subcategory,
    bool batteryPowered,
    const std::string& roomId,
    int codeLine ) {
    CoreD::Model::Device device =
        createDevice( coreRemoteInterface, name, type, deviceTypeId, category, subcategory, batteryPowered, codeLine );
    device.setRoomId( roomId );
    return addDevice( coreRemoteInterface, device, codeLine );
}

CoreDRemoteInterface::CoreDInterface::DeviceID CoreNmaTest::addDeviceWithRoom(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& name,
    const std::string& type,
    const std::string& deviceTypeId,
    const std::string& category,
    const std::string& subcategory,
    bool batteryPowered,
    const std::string& parentId,
    const std::string& roomId,
    bool parenRoom,
    int codeLine ) {
    CoreD::Model::Device device =
        createDevice( coreRemoteInterface, name, type, deviceTypeId, category, subcategory, batteryPowered, codeLine );
    device.setParentDeviceId( parentId );
    device.setRoomId( roomId );
    device.setParentRoom( parenRoom );
    return addDevice( coreRemoteInterface, device, codeLine );
}

CoreDRemoteInterface::CoreDInterface::DeviceID CoreNmaTest::addDevice(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const CoreD::Model::Device& device,
    int codeLine ) {
    JsonObject broadcast, result;
    std::string codeLineStr = getCodeLineString( codeLine );
    auto deviceAddingResult = coreRemoteInterface.addDevice( device );
    assertTrue( deviceAddingResult.second, codeLineStr );
    broadcast = waitForBroadcast( "hub.device.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "serviceNotification" ), false ) << codeLineStr;
    return deviceAddingResult.first;
}

void CoreNmaTest::removeDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                const std::string& deviceId,
                                size_t itemNumber,
                                size_t settingNumber,
                                int codeLine ) {
    JsonObject broadcast;
    std::string codeLineStr = getCodeLineString( codeLine );
    coreRemoteInterface.removeDevice( deviceId );
    for( size_t i = 0; i < itemNumber; ++i ) {
        broadcast = waitForBroadcast( "hub.item.removed", codeLineStr );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" ) << codeLineStr;
    }

    for( size_t i = 0; i < settingNumber; ++i ) {
        broadcast = waitForBroadcast( "hub.device.setting.removed", codeLineStr );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.setting.removed" ) << codeLineStr;
    }

    broadcast = waitForBroadcast( "hub.device.removed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" ) << codeLineStr;
}

void CoreNmaTest::removeItem( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                              const std::string& itemId,
                              int codeLine ) {
    JsonObject broadcast;
    std::string codeLineStr = getCodeLineString( codeLine );
    coreRemoteInterface.removeItem( itemId );
    broadcast = waitForBroadcast( "hub.item.removed", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" ) << codeLineStr;
}

void CoreNmaTest::removePolicy( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                const std::string& policyId,
                                int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    Json policyCommand;
    json_add_str( policyCommand, "id", policyId );

    LOGGER_WARN( "DEV: PolicyOperations::delete command: " + cc::j( policyCommand ) );

    auto interface = coreRemoteInterface.policyOperations();
    auto policyResult = interface->remove( policyCommand );

    auto broadcast = waitForBroadcast( "hub.policy.deleted", NMA_TEST_MAKE_MESSAGE( "PolicyRemoteInterface::delete" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.policy.deleted" ) << codeLineStr;
}

CoreD::Model::Device CoreNmaTest::createDevice( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                                const std::string& name,
                                                const std::string& type,
                                                const std::string& deviceTypeId,
                                                const std::string& category,
                                                const std::string& subcategory,
                                                bool batteryPowered,
                                                int codeLine ) {
    auto gatewayId = getMainGateway( coreRemoteInterface, codeLine );
    CoreD::Model::Device device( gatewayId, name, type, deviceTypeId, category, batteryPowered );
    device.setSubcategory( subcategory );
    return device;
}

std::string CoreNmaTest::getMainGateway( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    auto gateways = coreRemoteInterface.getGateways();
    assertTrue( !gateways.empty(), codeLineStr );
    for( const auto& gateway : gateways ) {
        if( json_get_str( gateway, "name" ) == "model_factory" ) {
            return json_get_str( gateway, "_id" );
        }
    }
    assertTrue( false, codeLineStr );
    return std::string();
}

void CoreNmaTest::dumpDevices( const JsonObject& devicesArray ) {
    if( !devicesArray.empty() ) {
        Log::debug( cc::debug( "Devices found, dump: " ) + cc::j( devicesArray ) );
    }
}

JsonObject CoreNmaTest::fetchExistingDevices() {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );

    return remoteInterface.getDevices();
}

/* Gets device list and calls removeDeviceSync() for every deviceId
 * @return {JsonObject} devices array
 */
size_t CoreNmaTest::clearExistingDevices( int codeLine ) {
    JsonObject existingDevices = fetchExistingDevices();
    dumpDevices( existingDevices );

    size_t counter = 0;
    if( existingDevices.is_array() ) {
        using namespace CoreD::Model::Resources;
        CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );

        std::for_each( begin( existingDevices ), end( existingDevices ), [&]( JsonObject deviceInfo ) {
            if( deviceInfo.count( "_id" ) > 0 ) {
                std::string deviceId = deviceInfo[ "_id" ];

                const auto itemNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
                const auto settingsNumber = remoteInterface.getDeviceSettingIdsByDeviceId( deviceId ).size();
                this->removeDevice( remoteInterface, deviceId, itemNumber, settingsNumber, codeLine );
                counter++;
            }
        } );
    }
    return counter;
}

CoreDRemoteInterface::CoreDInterface::PolicyID CoreNmaTest::addPolicy(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& name,
    const std::string& protectConfig,
    const std::string& deviceGroup,
    const std::vector< std::string >& meshbots,
    bool persistent,
    int codeLine ) {
    Json policyCommand;
    json_add_str( policyCommand, "name", name );
    json_add_str( policyCommand, "protectConfig", protectConfig );
    json_add_str( policyCommand, "deviceGroup", deviceGroup );
    json_add_str_array( policyCommand, "meshbots", meshbots );
    json_add_bool( policyCommand, "persistent", persistent );

    LOGGER_WARN( "DEV: PolicyOperations::create command: " + cc::j( policyCommand ) );

    auto interface = coreRemoteInterface.policyOperations();
    auto policyResult = interface->create( policyCommand );

    auto broadcast = waitForBroadcast( "hub.policy.created", NMA_TEST_MAKE_MESSAGE( "PolicyRemoteInterface::create" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.policy.created" ) << getCodeLineString( codeLine );

    Json id;
    json_get( broadcast, { "result", "_id" }, id );
    return id;
}

void CoreNmaTest::dumpPolicies( const JsonObject& policiesArray ) {
    if( !policiesArray.empty() ) {
        Log::debug( cc::debug( "Policies found, dump: " ) + cc::j( policiesArray ) );
    }
}

CoreDRemoteInterface::CoreDInterface::ItemID CoreNmaTest::addItem(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& deviceId,
    const std::string& name,
    CoreD::Model::ValueType valueType,
    bool hasGetter,
    bool hasSetter,
    int codeLine ) {
    return addItem( coreRemoteInterface,
                    deviceId,
                    name,
                    std::move( valueType ),
                    hasGetter,
                    hasSetter,
                    CoreD::Model::Item::Enum(),
                    codeLine );
}

CoreDRemoteInterface::CoreDInterface::ItemID CoreNmaTest::addItem(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& deviceId,
    const std::string& name,
    const CoreD::Model::ValueType& valueType,
    bool hasGetter,
    bool hasSetter,
    const CoreD::Model::Item::Enum& enumList,
    int codeLine ) {
    JsonObject broadcast;
    CoreD::Model::Item item( deviceId,
                             name,
                             valueType,
                             hasGetter,
                             hasSetter,
                             CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );
    item.setEnum( enumList );

    return addItem( coreRemoteInterface, item, codeLine, nullptr );
}

CoreDRemoteInterface::CoreDInterface::ItemID CoreNmaTest::addItem(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const CoreD::Model::Item& item,
    int codeLine,
    BroadcastAssert assertPred ) {
    //
    JsonObject broadcast;
    const std::string codeLineStr = getCodeLineString( codeLine );

    auto itemAddingResult = coreRemoteInterface.addItem( item );
    assertTrue( itemAddingResult.second, codeLineStr );
    auto itemId = itemAddingResult.first;
    broadcast = waitForBroadcast( "hub.item.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" ) << codeLineStr;

    if( assertPred != nullptr )
        assertPred( broadcast, item );
    return itemId;
}

CoreDRemoteInterface::CoreDInterface::DeviceSettingId CoreNmaTest::addDeviceSetting(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& deviceId,
    CoreD::Model::ValueType valueType,
    int codeLine ) {
    JsonObject broadcast;
    const std::string codeLineStr = getCodeLineString( codeLine );

    CoreD::Model::DeviceSetting setting(
        deviceId, valueType, CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );

    const auto settingAddingResult = coreRemoteInterface.addDeviceSetting( setting );
    assertTrue( settingAddingResult.second, codeLineStr );

    auto settingId = settingAddingResult.first;
    broadcast = waitForBroadcast( "hub.device.setting.added", codeLineStr );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.setting.added" ) << codeLineStr;

    return settingId;
}

CoreDRemoteInterface::CoreDInterface::ItemID CoreNmaTest::addDictionary(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& deviceId,
    const std::string& name,
    CoreD::Model::ValueType valueType,
    bool hasGetter,
    bool hasSetter,
    int codeLine ) {
    using namespace CoreD::Model;
    std::string itemId;
    bool isSuccess;
    auto dictionaryType = ValueType::makeDictionaryType( valueType );
    Item dictionaryItem(
        deviceId, name, dictionaryType, hasGetter, hasSetter, ValueFactory::createDefaultItemValue( dictionaryType ) );
    std::tie( itemId, isSuccess ) = coreRemoteInterface.addItem( dictionaryItem );
    JsonObject broadcast;
    std::string codeLineStr = getCodeLineString( codeLine );
    assertTrue( isSuccess, codeLineStr );
    broadcast = waitForBroadcast( "hub.item.added", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.added" ) << codeLineStr;
    return itemId;
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithOneItem( CoreD::Model::ValueType::Enum valueType,
                                                                      int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    if( codeLine == -1 )
        codeLine = __LINE__;
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               codeLine );
    auto itemId = addItem( coreRemoteInterface, deviceId, "testItem1", valueType, true, true, codeLine );
    return { deviceId, itemId };
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithSecurityThreatItem( int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";
    auto deviceId = addDevice( coreRemoteInterface,
                               "Controller device-item group device",
                               deviceCategory,
                               deviceCategory,
                               deviceCategory,
                               deviceSubcategory,
                               false,
                               __LINE__ );
    auto itemId = addItem(
        coreRemoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, codeLine );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );
    return { deviceId, itemId };
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithSmokeSensorItem( int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";
    auto deviceId = addDevice( coreRemoteInterface,
                               "Controller device-item group device",
                               deviceCategory,
                               deviceCategory,
                               deviceCategory,
                               deviceSubcategory,
                               false,
                               __LINE__ );
    auto itemId = addItem(
        coreRemoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, codeLine );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );
    return { deviceId, itemId };
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithMotionSensorItem( int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "security_threat";
    auto deviceId = addDevice( coreRemoteInterface,
                               "Controller device-item group device",
                               deviceCategory,
                               deviceCategory,
                               deviceCategory,
                               deviceSubcategory,
                               false,
                               __LINE__ );
    auto itemId = addItem(
        coreRemoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, codeLine );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );
    return { deviceId, itemId };
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithOneItemAndRoom( CoreD::Model::ValueType::Enum valueType,
                                                                             const std::string& roomId,
                                                                             int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    if( codeLine == -1 )
        codeLine = __LINE__;
    auto deviceId = addParentDeviceWithRoom( coreRemoteInterface,
                                             "testDevice1",
                                             "smoke",
                                             "134_3_24",
                                             "security_sensor",
                                             "smoke",
                                             false /*batteryPowered*/,
                                             roomId,
                                             codeLine );

    auto itemId = addItem( coreRemoteInterface, deviceId, "testItem1", valueType, true, true, codeLine );
    return { deviceId, itemId };
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addChildDeviceWithOneItem( CoreD::Model::ValueType::Enum valueType,
                                                                           const std::string& parentId,
                                                                           int codeLine ) {
    return addChildDeviceWithOneItemAndParentRoom( valueType, parentId, true, "roomID", codeLine );
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addChildDeviceWithOneItemAndParentRoom(
    CoreD::Model::ValueType::Enum valueType,
    const std::string& parentId,
    bool parentRoom,
    const std::string roomId,
    int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    if( codeLine == -1 )
        codeLine = __LINE__;
    auto deviceId = addDeviceWithRoom( coreRemoteInterface,
                                       "testDevice1",
                                       "smoke",
                                       "134_3_24",
                                       "security_sensor",
                                       "smoke",
                                       false /*batteryPowered*/,
                                       parentId,
                                       roomId,
                                       parentRoom,
                                       codeLine );
    auto itemId = addItem( coreRemoteInterface, deviceId, "testItem1", valueType, true, true, codeLine );

    return { deviceId, itemId };
}

void CoreNmaTest::setItemValue( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                const std::string& itemId,
                                const JsonObject& value,
                                int codeLine,
                                JsonObject scale ) {
    using namespace CoreD::Model::Resources;
    auto request = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
    if( !scale.empty() ) {
        request[ "scale" ] = scale;
    }
    coreRemoteInterface.setItemValue( request );
    checkItemUpdateBroadcast( itemId, value, codeLine );
}

void CoreNmaTest::setItemValueWithoutCheck( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                            const std::string& itemId,
                                            const JsonObject& value,
                                            int codeLine,
                                            JsonObject scale ) {
    using namespace CoreD::Model::Resources;
    auto request = JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << value );
    if( !scale.empty() ) {
        request[ "scale" ] = scale;
    }
    coreRemoteInterface.setItemValue( request );
}

void CoreNmaTest::runScript( const std::string& scriptPath, const JsonObject& params, int codeLine ) {
    runScript( scriptPath, "finished", params, codeLine );
}

void CoreNmaTest::runFailedScript( const std::string& scriptPath, int codeLine ) {
    runScript( scriptPath, "failed", {}, codeLine );
}

void CoreNmaTest::runFailedScript( const std::string& scriptPath, const JsonObject& params, int codeLine ) {
    runScript( scriptPath, "failed", params, codeLine );
}

void CoreNmaTest::runScript( const std::string& scriptPath,
                             const std::string& result,
                             const JsonObject& params,
                             int codeLine ) {
    runScriptWithoutChecks( scriptPath, params, codeLine );

    checkExtensionsPluginRunProgressBroadcast( "started", codeLine );
    checkExtensionsPluginRunProgressBroadcast( result, codeLine );
}

void CoreNmaTest::runScriptCheckReason( const std::string& scriptPath,
                                        const JsonObject& params,
                                        const std::string& reason,
                                        int codeLine ) {
    runScriptWithoutChecks( scriptPath, params, codeLine );

    checkExtensionsPluginRunProgressBroadcast( "started", codeLine );
    checkExtensionsPluginRunProgressBroadcastByReason( "failed", reason, codeLine );
}

void CoreNmaTest::runScriptWithoutChecks( const std::string& scriptPath, const JsonObject& params, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    auto scriptWithParams = replaceTokens( hubExtensionsPluginRunWithParams,
                                           { { "__SCRIPT__", scriptPath }, { "__PARAMS__", params.dump() } } );
    auto answer = nma().sendtoHubAndGetAnswer( scriptWithParams );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << ", " << codeLineStr;
}

void CoreNmaTest::checkDeviceUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::DeviceID deviceId,
                                              bool isReady,
                                              int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast, result;
    broadcast = waitForBroadcast( "hub.device.updated", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "_id" ), deviceId ) << codeLineStr;
    EXPECT_EQ( json_get_bool( result, "ready", !isReady ), isReady ) << codeLineStr;
}

void CoreNmaTest::checkDeviceUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::DeviceID deviceId,
                                              const KeyJsonList& fields,
                                              int codeLine ) {
    checkBroadcast( deviceId, "hub.device.updated", fields, codeLine );
}

void CoreNmaTest::checkItemDictionaryUpdatedBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                                       const KeyJsonList& fields,
                                                       int codeLine ) {
    checkBroadcast( itemId, "hub.item.dictionary.updated", fields, codeLine );
}

void CoreNmaTest::checkUserNotificationBroadcast( const std::string& deviceName,
                                                  const std::string& roomName,
                                                  const std::string& user,
                                                  int codeLine ) {
    JsonObject broadcast;
    auto codeLineStr = getCodeLineString( codeLine );
    broadcast = waitForBroadcast( "hub.user.notification", codeLineStr );
    checkUserNotificationBroadcast( broadcast, deviceName, roomName, user, codeLine );
}

void CoreNmaTest::checkUserNotificationBroadcast( const JsonObject& broadcast,
                                                  const std::string& deviceName,
                                                  const std::string& roomName,
                                                  const std::string& user,
                                                  int codeLine ) {
    JsonObject result, params;
    auto codeLineStr = getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.user.notification" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "type" ), "pin_code_is_used" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( result, "params", params ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( params, "deviceName" ), deviceName ) << codeLineStr;
    EXPECT_EQ( json_get_str( params, "roomName" ), roomName ) << codeLineStr;
    EXPECT_EQ( json_get_str( params, "user" ), user ) << codeLineStr;
}

void CoreNmaTest::checkExtensionsPluginRunProgressBroadcast( const std::string& status, int codeLine ) {
    JsonObject broadcast;
    auto codeLineStr = getCodeLineString( codeLine );
    broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", codeLineStr );
    checkExtensionsPluginRunProgressBroadcast( broadcast, status, codeLine );
}

void CoreNmaTest::checkExtensionsPluginRunProgressBroadcastByReason( const std::string& status,
                                                                     const std::string& reason,
                                                                     int codeLine ) {
    JsonObject broadcast;
    auto codeLineStr = getCodeLineString( codeLine );
    broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", codeLineStr );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    JsonObject error;
    EXPECT_TRUE( json_get_obj( result, "error", error ) ) << codeLineStr;
    auto aReason = json_get_str( error, "reason" );
    EXPECT_TRUE( aReason.find( reason ) != std::string::npos ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
}

void CoreNmaTest::checkExtensionsPluginRunProgressBroadcast( const JsonObject& broadcast,
                                                             const std::string& status,
                                                             int codeLine ) {
    JsonObject result;
    auto codeLineStr = getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.run.progress" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), status ) << codeLineStr;
}

double CoreNmaTest::toFahrenheit( double inCelsius ) {
    using namespace HubPlatform::CoreD::Model;
    const auto valueType = ValueType::Enum::Temperature;
    const Json valueData = { { "value", inCelsius }, { "scale", ValueScales::Celsius } };
    auto ptr = ScalableValue::createValue( valueType, valueData );
    return ptr->getConvertedValue( ValueScales::Fahrenheit );
}

double CoreNmaTest::toCelsius( double inFahrenheit ) {
    using namespace HubPlatform::CoreD::Model;
    const auto valueType = ValueType::Enum::Temperature;
    const Json valueData = { { "value", inFahrenheit }, { "scale", ValueScales::Fahrenheit } };
    auto ptr = ScalableValue::createValue( valueType, valueData );
    return ptr->getConvertedValue( ValueScales::Fahrenheit );
}

double CoreNmaTest::convertToScale( double inScale, const char* fromScale, const char* toScale ) {
    using namespace HubPlatform::CoreD::Model;
    const auto valueType = ValueType::Enum::Temperature;
    const Json valueData = { { "value", inScale }, { "scale", fromScale } };
    auto ptr = ScalableValue::createValue( valueType, valueData );
    return ptr->getConvertedValue( toScale );
}

void CoreNmaTest::compareTemperatureValues( const Json& itemInfo,
                                            const Json& response,
                                            const std::string& responseKey,
                                            const std::string& message ) {
    auto itemValue = json_get_float( itemInfo, "value" );
    auto itemScale = json_get_str( itemInfo, "scale" );
    auto responseValue = json_get_float( response, responseKey );
    auto responseScale = json_get_str( response, "scale" );

    if( itemScale == responseScale ) {
        EXPECT_FLOAT_EQ( itemValue, responseValue );
    }
    else {
        CoreD::Model::ValueType valueType( CoreD::Model::ValueType::Enum::Temperature );
        auto converted = valueType.getConverter()( itemValue, itemScale.c_str(), responseScale.c_str() );
        EXPECT_FLOAT_EQ( converted, responseValue )
            << ( message.empty() ? "Temperature values should match when converted to the same scale" : message );
    }
}

void CoreNmaTest::checkResult( const Json& request, const std::string& expectedValue, int line ) {
    const std::string codeLineStr = getCodeLineString( line );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << " " << codeLineStr;

    checkUiBroadcast( expectedValue, line );

    checkExtensionsPluginRunProgressBroadcast( "started", line );
    checkExtensionsPluginRunProgressBroadcast( "finished", line );
}

void CoreNmaTest::checkFailure( const Json& request, const std::string& errorMessage, int line ) {
    const std::string codeLineStr = getCodeLineString( line );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << " " << codeLineStr;

    checkExtensionsPluginRunProgressBroadcast( "started", line );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", codeLineStr );
    Json result;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.run.progress" ) << codeLineStr;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << codeLineStr;
    EXPECT_EQ( json_get_str( result, "status" ), "failed" ) << codeLineStr;

    std::string reason;
    ASSERT_TRUE( json_get( result, std::vector< Json >{ "error", "reason" }, reason ) ) << codeLineStr;
    EXPECT_TRUE( reason.find( errorMessage ) != std::string::npos ) << codeLineStr;
}

void CoreNmaTest::checkUiBroadcast( const std::string& expectedValue, int line ) {
    const std::string codeLineStr = getCodeLineString( line );
    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", codeLineStr );
    ASSERT_FALSE( broadcast.empty() ) << codeLineStr;
    JsonObject params;
    ASSERT_TRUE( json_get_obj( broadcast, "result", params ) ) << codeLineStr;
    if( !expectedValue.empty() ) {
        auto value = json_get_str( params, "value" );
        ASSERT_FALSE( value.empty() ) << codeLineStr;
        ASSERT_EQ( value, expectedValue ) << codeLineStr;
    }
}

bool CoreNmaTest::isHouseModeSwitchedBroadcast( const JsonObject& broadcast,
                                                const std::string& toMode,
                                                const std::string& status ) {
    JsonObject result;
    return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.switched" &&
           json_get_obj( broadcast, "result", result ) && json_get_str( result, "to" ) == toMode &&
           json_get_str( result, "status" ) == status;
}

bool CoreNmaTest::isUserNotificationBroadcast( const JsonObject& broadcast,
                                               const std::string& deviceName,
                                               const std::string& roomName,
                                               const std::string& user ) const {
    JsonObject result, params;
    return json_get_str( broadcast, "msg_subclass" ) == "hub.user.notification" &&
           json_get_obj( broadcast, "result", result ) && json_get_obj( result, "params", params ) &&
           json_get_str( params, "deviceName" ) == deviceName && json_get_str( params, "roomName" ) == roomName &&
           json_get_str( params, "user" ) == user;
}

void CoreNmaTest::setDefaultTemperatureScaleSetting( const std::string& scale, int codeLine ) {
    Json hubSettingValueSet = { { "method", "hub.setting.value.set" },
                                { "params", { { "name", "scale.temperature" }, { "value", scale } } } };

    CoreDRemoteInterface::CoreDInterface& remote = makeRemote( nma() );

    auto settingPtr = remote.findSetting( "scale.temperature" );
    if( !settingPtr ) {
        Log::error( cc::error( "Setting scale.temperature could not be fetched" ) );
        return;
    }

    auto settingValue = settingPtr->getTokenValue();
    if( settingValue != scale ) {
        settingPtr->setTokenValue( scale );
        remote.saveSetting( *settingPtr );

        Json broadcast = waitForBroadcast( "hub.setting.updated", getCodeLineString( codeLine ) );
        // hub.settings.updated
        EXPECT_EQ( broadcast[ "msg_subclass" ], "hub.setting.updated" );
        EXPECT_EQ( broadcast[ "result" ][ "value" ], scale );
    }
}

CoreDRemoteInterface::CoreDInterface& CoreNmaTest::makeRemote( Services::Service& hostService ) {
    static auto ptr = HubPlatform::CoreDRemoteInterface::makeInstance( hostService );
    return *ptr;
}

CoreDRemoteInterface::CoreDInterface& CoreNmaTest::makeRemote() {
    static auto ptr = HubPlatform::CoreDRemoteInterface::makeInstance( nma() );
    return *ptr;
}

CoreNmaTest::DeviceWithOneItemData CoreNmaTest::addDeviceWithProtectAlarmItem( int codeLine ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    const std::string deviceCategory = "sos_button";
    const std::string deviceSubcategory = "police_alarm";
    const std::string& testItemName = "protect_alarm";
    auto deviceId = addDevice( coreRemoteInterface,
                               "Controller device-item group device",
                               deviceCategory,
                               deviceCategory,
                               deviceCategory,
                               deviceSubcategory,
                               false,
                               __LINE__ );
    auto itemId = addItem(
        coreRemoteInterface, deviceId, testItemName, CoreD::Model::ValueType::Enum::Token, true, true, codeLine );

    auto answer =
        coreRemoteInterface.houseModesProtectButtonsSet( deviceId, CoreDHouseModes::ProtectServiceEnum::POLICE );

    Json result;
    Json broadcast = waitForBroadcast( "hub.modes.protect.buttons.added",
                                       NMA_TEST_MAKE_MESSAGE( "hub.modes.protect.buttons.added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.protect.buttons.added" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId );

    EXPECT_EQ( json_get_str( result, "service" ), "police" );

    return { deviceId, itemId };
}

CoreDRemoteInterface::CoreDInterface::DeviceGroupId CoreNmaTest::addDeviceGroup(
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
    const std::string& name,
    const bool persistent,
    const std::string& entryDelay,
    const bool followEntry,
    const std::string& role,
    const std::string& packageId,
    const std::vector< std::string >& devices,
    const std::vector< std::string >& exceptions,
    int codeLine ) {
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "name", name );
    json_add_bool( deviceGroupCommand, "persistent", persistent );
    json_add_str( deviceGroupCommand, "entryDelay", entryDelay );
    json_add_bool( deviceGroupCommand, "followEntry", followEntry );
    json_add_str( deviceGroupCommand, "role", role );
    json_add_str( deviceGroupCommand, "packageId", packageId );
    json_add_str_array( deviceGroupCommand, "devices", devices );
    json_add_str_array( deviceGroupCommand, "exceptions", exceptions );

    LOGGER_WARN( "DEV: DeviceGroupOperations::create command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.create( deviceGroupCommand );

    auto broadcast =
        waitForBroadcast( "hub.device.group.created", NMA_TEST_MAKE_MESSAGE( "DeviceGroupRemoteInterface::create" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" ) << getCodeLineString( codeLine );
    m_lastDeviceGroupBroadcast = broadcast;

    Json id;
    json_get( broadcast, { "result", "_id" }, id );
    return id;
}

void CoreNmaTest::updateDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                     const std::string& id,
                                     const std::string& name,
                                     const bool persistent,
                                     const std::string& entryDelay,
                                     const bool followEntry,
                                     const std::vector< std::string >& devices,
                                     const std::vector< std::string >& exceptions,
                                     int codeLine ) {
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "id", id );
    json_add_str( deviceGroupCommand, "name", name );
    json_add_bool( deviceGroupCommand, "persistent", persistent );
    json_add_str( deviceGroupCommand, "entryDelay", entryDelay );
    json_add_bool( deviceGroupCommand, "followEntry", followEntry );
    json_add_str_array( deviceGroupCommand, "devices", devices );
    json_add_str_array( deviceGroupCommand, "exceptions", exceptions );

    LOGGER_WARN( "DEV: DeviceGroupOperations::update command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.update( deviceGroupCommand );

    auto broadcast =
        waitForBroadcast( "hub.device.group.updated", NMA_TEST_MAKE_MESSAGE( "DeviceGroupRemoteInterface::update" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" ) << getCodeLineString( codeLine );
    m_lastDeviceGroupBroadcast = broadcast;
}

void CoreNmaTest::removeDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                     const std::string& deviceGroupId,
                                     int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "id", deviceGroupId );

    LOGGER_WARN( "DEV: DeviceGroupOperations::delete command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.deleteGroup( deviceGroupCommand );

    auto broadcast =
        waitForBroadcast( "hub.device.group.deleted", NMA_TEST_MAKE_MESSAGE( "DeviceGroupRemoteInterface::delete" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.deleted" ) << codeLineStr;
    m_lastDeviceGroupBroadcast = broadcast;
}

JsonObject CoreNmaTest::getDeviceGroup( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                        const std::string& deviceGroupId,
                                        int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "id", deviceGroupId );

    LOGGER_WARN( "DEV: DeviceGroupOperations::get command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.get( deviceGroupCommand );

    return deviceGroupResult;
}

void CoreNmaTest::addDeviceGroupDevices( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                         const std::string& deviceGroupId,
                                         const std::vector< std::string >& devices,
                                         int codeLine ) {
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "id", deviceGroupId );
    json_add_str_array( deviceGroupCommand, "devices", devices );

    LOGGER_WARN( "DEV: DeviceGroupOperations::add devices command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.addDevices( deviceGroupCommand );

    auto broadcast =
        waitForBroadcast( "hub.device.group.updated", NMA_TEST_MAKE_MESSAGE( "DeviceGroupRemoteInterface::update" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" ) << getCodeLineString( codeLine );
    m_lastDeviceGroupBroadcast = broadcast;
}

void CoreNmaTest::removeDeviceGroupDevices( CoreDRemoteInterface::CoreDInterface& coreRemoteInterface,
                                            const std::string& deviceGroupId,
                                            const std::vector< std::string >& devices,
                                            int codeLine ) {
    Json deviceGroupCommand;
    json_add_str( deviceGroupCommand, "id", deviceGroupId );
    json_add_str_array( deviceGroupCommand, "devices", devices );

    LOGGER_WARN( "DEV: DeviceGroupOperations::remove devices command: " + cc::j( deviceGroupCommand ) );

    auto interface = coreRemoteInterface.groupOperations()->deviceGroup();
    auto deviceGroupResult = interface.removeDevices( deviceGroupCommand );

    auto broadcast =
        waitForBroadcast( "hub.device.group.updated", NMA_TEST_MAKE_MESSAGE( "DeviceGroupRemoteInterface::update" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" ) << getCodeLineString( codeLine );
    m_lastDeviceGroupBroadcast = broadcast;
}

JsonObject CoreNmaTest::getLastDeviceGroupBroadcast() {
    return m_lastDeviceGroupBroadcast;
}

}  // namespace Tests
}  // namespace HubPlatform
