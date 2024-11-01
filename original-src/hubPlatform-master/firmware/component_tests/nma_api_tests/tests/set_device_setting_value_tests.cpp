#include "nma_api_tests/application.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/values.h"
#include "jsonrpc/jsonrpc.h"

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace Services;

using MakeRequest =
    std::function< JsonObject( std::string const& itemId, JsonObject const& value, JsonObject const& scale ) >;
using AssertionPred = std::function< void( std::string itemId, JsonObject const& value, int codeLine ) >;

static JsonObject makeRequestDefault( std::string const& itemId, JsonObject const& value, JsonObject const& scale );

class SetDeviceSettingValueFixture : public CoreNmaTest {
public:
    SetDeviceSettingValueFixture();
    static void SetUpTestCase();
    static void TearDownTestCase();

    void SetUp() override;
    void TearDown() override;

    virtual void TestBody() override{};

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;

    using SettingNameToIdMap = std::map< std::string, std::string >;
    using DeviceSettingsMap = std::map< std::string, SettingNameToIdMap >;
    static DeviceSettingsMap& deviceSettings() {
        static DeviceSettingsMap dict;
        return dict;
    }
};

SetDeviceSettingValueFixture::SetDeviceSettingValueFixture()
: remoteInterface( makeRemote( nma() ) ) {}

void SetDeviceSettingValueFixture::SetUpTestCase() {
    Log::trace( "testSuite: SetUp" );

    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    SetDeviceSettingValueFixture coreTest;
    coreTest.CommonNmaTests::SetUp();

    // create device
    std::string deviceId = coreTest.addDevice( remoteInterface,
                                               "testDevice",
                                               "smoke",
                                               "134_3_24",
                                               "security_sensor",
                                               "smoke",
                                               false /*batteryPowered*/,
                                               __LINE__ );

    auto& mapDeviceItems = deviceSettings();
    auto& deviceItems = mapDeviceItems[ deviceId ];

    // create items for test
    using namespace Model::Resources;

    {
        auto settingId = coreTest.addDeviceSetting(
            remoteInterface, deviceId, CoreD::Model::ValueType::Enum::Temperature, __LINE__ );

        deviceItems[ "item_temperature" ] = std::move( settingId );
    }

    {
        auto settingId =
            coreTest.addDeviceSetting( remoteInterface, deviceId, CoreD::Model::ValueType::Enum::Action, __LINE__ );
        deviceItems[ "setting_action" ] = std::move( settingId );
    }

    coreTest.CommonNmaTests::TearDown();
}

void SetDeviceSettingValueFixture::TearDownTestCase() {
    Log::trace( "testSuite: TearDown" );
    SetDeviceSettingValueFixture coreTest;

    // Call it to wait of all broadcasts passed after the tests done
    coreTest.CommonNmaTests::SetUp();

    coreTest.clearExistingDevices( __LINE__ );
    auto& currentMapping = deviceSettings();
    currentMapping.clear();
}

void SetDeviceSettingValueFixture::SetUp() {
    Log::trace( "testCase: SetUp" );
    CoreNmaTest::SetUp();
}

void SetDeviceSettingValueFixture::TearDown() {
    Log::trace( "testCase: SetUp" );
    CoreNmaTest::TearDown();
}

class SetDeviceSettingValueTest : public SetDeviceSettingValueFixture {
public:
    void SetUp() override {
        SetDeviceSettingValueFixture::SetUp();
        Log::trace( "SetItemValueTypeTest::SetUp" );
    }
    void TearDown() override {
        SetDeviceSettingValueFixture::TearDown();
        Log::trace( "SetItemValueTypeTest::TearDown" );
    }

    static void SetUpTestCase() {
        Log::trace( "testSuite: SetUpTestCase" );
        SetDeviceSettingValueFixture::SetUpTestCase();
    }
    static void TearDownTestCase() {
        Log::trace( "testSuite: TearDownTestCase" );
        SetDeviceSettingValueFixture::TearDownTestCase();
    }
};

static JsonObject makeRequestDefault( const std::string& itemId, const JsonObject& value, const JsonObject& scale ) {
    using namespace CoreD::Model::Resources;
    auto request = JSON( DeviceSettingFields::Id << itemId );

    if( !value.is_null() ) {
        request[ DeviceSettingFields::Value ] = value;
    }

    if( !scale.empty() ) {
        request[ "scale" ] = scale;
    }
    return request;
}

TEST_F( SetDeviceSettingValueTest, SetActionValue ) {
    ASSERT_FALSE( deviceSettings().empty() ) << "Invalid test configuration: no devices set up!";

    const auto settingId = deviceSettings().begin()->second[ "setting_action" ];

    EXPECT_NO_THROW( remoteInterface.setSettingValue( makeRequestDefault( settingId, {}, {} ) ) );
    EXPECT_NO_THROW( remoteInterface.setSettingValue( makeRequestDefault( settingId, Json::object(), {} ) ) );
    EXPECT_NO_THROW( remoteInterface.setSettingValue( makeRequestDefault( settingId, "", {} ) ) );
    EXPECT_NO_THROW( remoteInterface.setSettingValue( makeRequestDefault( settingId, 0, {} ) ) );
}

TEST_F( SetDeviceSettingValueTest, ResetSettingsValuesByDeviceIdValue ) {
    ASSERT_FALSE( deviceSettings().empty() ) << "Invalid test configuration: no devices set up!";

    const auto deviceId = deviceSettings().begin()->first;

    EXPECT_NO_THROW( remoteInterface.resetSetting( JSON( "deviceId" << deviceId ) ) );
}

}  // namespace Tests
}  // namespace HubPlatform
