#include "house_mode_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace Tests {

void TestsHouseModeFixture::SetUp() {
    DeviceItemGroupsFixture::SetUp();
    resetDisarmedDefaultFlag();
}

void TestsHouseModeFixture::TearDown() {
    clearExistingDevices( __LINE__ );
    DeviceItemGroupsFixture::TearDown();
}

JsonObject TestsHouseModeFixture::callHouseModesGet( const std::string& errorMessage ) {
    auto answer = HubPlatform::Tests::CommonNmaTests::nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ } } );
    return answer;
}

void TestsHouseModeFixture::checkHouseModes( const Json& commandResponse,
                                             const HubModesGetPred& modesPred,
                                             const std::string& errorMessage ) {
    JsonObject result;
    JsonArray houseModes;
    ASSERT_TRUE( json_get_obj( commandResponse, "result", result ) ) << errorMessage;
    ASSERT_TRUE( json_get_arr( result, "modes", houseModes ) ) << errorMessage;
    for( const auto& mode : houseModes ) {
        if( modesPred != nullptr )
            modesPred( mode, errorMessage );
    }
}

bool TestsHouseModeFixture::findInCollection( const Json& modesGetAnswer,
                                              const std::string& collectionName,
                                              const std::string& deviceId ) {
    Json result, collection;
    if( json_get_obj( modesGetAnswer, "result", result ) && json_get_arr( result, collectionName, collection ) ) {
        auto foundIt = std::find_if( begin( collection ), end( collection ), [&deviceId]( const Json& json ) {
            auto id = json_get_str( json );
            if( id.empty() )
                return false;
            return deviceId == id;
        } );
        return foundIt != end( collection );
    }
    return false;
}

void TestsHouseModeFixture::checkHouseModesGet( const HubModesGetPred& modesPred,
                                                const std::string& errorMessage,
                                                Json* answerReturned ) {
    auto answer = callHouseModesGet( errorMessage );
    JsonObject result;
    JsonArray houseModes;
    ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << errorMessage;
    ASSERT_TRUE( json_get_arr( result, "modes", houseModes ) ) << errorMessage;
    for( const auto& mode : houseModes ) {
        if( modesPred != nullptr )
            modesPred( mode, errorMessage );
    }
    if( answerReturned != nullptr ) {
        *answerReturned = std::move( answer );
    }
}

void TestsHouseModeFixture::checkModesChangedDisarmedDefault( bool value, const std::string& message ) {
    Json broadcast = HubPlatform::Tests::CommonNmaTests::waitForBroadcast( "hub.modes.changed", message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" ) << message;
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "disarmedDefault", false ), value ) << message;
}
void TestsHouseModeFixture::checkHouseModeSwitched( const std::string& message ) {
    auto broadcast = HubPlatform::Tests::CommonNmaTests::waitForBroadcast( "hub.modes.switched", message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.switched" ) << message;
}
void TestsHouseModeFixture::resetDisarmedDefaultFlag() {
    HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface remote( HubPlatform::Tests::CommonNmaTests::nma() );
    auto houseModes = remote.getHouseModes();
    for( auto& mode : houseModes ) {
        if( !mode.m_disarmedDefault ) {
            setDisarmedDefault( mode.m_id, true, std::string() );
        }
    }
}
void TestsHouseModeFixture::setDisarmedDefault( const std::string& modeId,
                                                bool disarmedDefault,
                                                const std::string& message ) {
    const auto answer = HubPlatform::Tests::CommonNmaTests::nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.disarmed_default.set" },
          { "params", JSON( "modeId" << modeId << "disarmedDefault" << disarmedDefault ) } } );
    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    EXPECT_TRUE( answer[ "error" ].empty() ) << message;

    checkModesChangedDisarmedDefault( disarmedDefault, message );
}

bool TestsHouseModeFixture::searchInArray( const JsonArray& collection, const Json& value ) {
    if( collection.is_array() && !collection.empty() ) {
        auto foundIt =
            find_if( begin( collection ), end( collection ), [&value]( const Json& elem ) { return elem == value; } );
        return foundIt != end( collection );
    }
    return false;
}

JsonArray TestsHouseModeFixture::fetchCollection( const Json& answer, const std::string& name ) {
    Json result, collection;
    if( json_get_obj( answer, "result", result ) )
        json_get_arr( result, name, collection );
    return collection;
}

}  // namespace Tests
}  // namespace HubPlatform
