#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "nma_api_tests/tests/fixtures/house_mode_fixture.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

----    1. House modes by name
DONE    1.1 Switch to every home mode in cycle
DONE    1.2 Try to switch to the same house mode
DONE    1.3 Try to switch to the house mode which doesn't match any existent one
DONE    1.4 Try to switch using empty 'name' parameter.
TODO    1.5 Mock house modes with ambiguous names and try to switch to it.

*/

class TestsHouseModeByName : public TestsHouseModeFixture {
public:
    TestsHouseModeByName();

    void SetUp() override;
    void TearDown() override;

    typedef std::function< void( JsonObject const& switchModeAnswer,
                                 std::string const& modeName,
                                 std::string const& modeId,
                                 int codeLine ) >
        AssertCallback;

    JsonObject switchToMode( const std::string& nextName, int codeLine = __LINE__ );
    void assertResults( JsonObject const& switchModeAnswer,
                        std::string const& modeId,
                        std::string const& modeName,
                        int codeLine,
                        AssertCallback callback );
    void handleBroadcasts( const std::string& modeId, int codeLine );

protected:
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface;
};

using namespace CoreDHouseModes::Resources;

TestsHouseModeByName::TestsHouseModeByName()
: remoteInterface( nma().get() ) {}

void TestsHouseModeByName::SetUp() {
    TestsHouseModeFixture::SetUp();
    // set switch_to_delay to 0 before running the test
    remoteInterface.setSwitchDelay( 0 );
}
void TestsHouseModeByName::TearDown() {
    TestsHouseModeFixture::TearDown();
    remoteInterface.setSwitchDelay( 30 );
}

JsonObject TestsHouseModeByName::switchToMode( const std::string& modeName, int codeLine ) {
    JsonObject request = { { "method", "hub.modes.switch" }, { "params", JSON( "name" << modeName ) } };
    JsonObject switchModeAnswer = nma().get().sendtoHubAndGetAnswer( request );

    return switchModeAnswer;
}

void TestsHouseModeByName::handleBroadcasts( const std::string& modeId, int codeLine ) {
    testHouseModesSwitchedBroadcast( modeId, "begin", codeLine );
    testHouseModesSwitchedBroadcast( modeId, "done", codeLine );
}

void TestsHouseModeByName::assertResults( JsonObject const& switchModeAnswer,
                                          std::string const& modeName,
                                          std::string const& modeId,
                                          int codeLine,
                                          AssertCallback callback ) {
    return callback( switchModeAnswer, modeName, modeId, codeLine );
}

TEST_F( TestsHouseModeByName, HouseModeSwitchToEveryMode /* 1.1 */ ) {
    using HouseModesGetResult = CoreDRemoteInterface::CoreDInterface::HouseModesGetResult;

    HouseModesGetResult houseModes = remoteInterface.getHouseModes();
    auto currentHouseModeId = remoteInterface.getCurrentHouseModeId();

    // change mode to the next one in list: try without return to 1-st element: it < sz
    for( size_t it = 0, sz = houseModes.size(); sz > 0 && it < sz; it++ ) {
        const auto& next = houseModes[ ( it + 1 ) % sz ];
        auto currentHouseModeId = remoteInterface.getCurrentHouseModeId();

        auto answer = switchToMode( next.m_name, __LINE__ );
        try {
            auto respDump = answer.dump();

            if( currentHouseModeId == next.m_id ) {
                // expect error, INVALID_PARAMS
                ASSERT_FALSE( answer.is_object() && answer[ "error" ] == nullptr )
                    << respDump << ", mode to: " << next.m_name;
            }
            else {
                // expect error to be null
                ASSERT_TRUE( answer.is_object() && answer[ "error" ] == nullptr )
                    << respDump << ", mode to: " << next.m_name;
            }
        }
        catch( std::exception const& e ) {
            Log::error( cc::error( "Exception: " ) + e.what() );
            FAIL() << "Exception occurred: " << e.what();
        }
        handleBroadcasts( next.m_id, __LINE__ );
    }
}

//    DONE    1.2 Try to switch to the same house mode
TEST_F( TestsHouseModeByName, HouseModeSwitchToTheSameMode /* 1.2 */ ) {
    using HouseModesGetResult = CoreDRemoteInterface::CoreDInterface::HouseModesGetResult;

    HouseModesGetResult houseModes = remoteInterface.getHouseModes();
    auto currentHouseModeId = remoteInterface.getCurrentHouseModeId();

    auto foundIt = std::find_if(
        begin( houseModes ), end( houseModes ), [&]( HouseModesGetResult::value_type const& entry ) -> bool {
            return entry.m_id == currentHouseModeId;
        } );

    ASSERT_TRUE( foundIt != end( houseModes ) ) << "House mode should exist";
    if( foundIt != end( houseModes ) ) {
        auto assertCallback = []( JsonObject const& switchModeAnswer,
                                  std::string const& modeName,
                                  std::string const& modeId,
                                  int codeLine ) {
            // some function
            try {
                ASSERT_TRUE( switchModeAnswer.is_object() && switchModeAnswer[ "error" ] != nullptr )
                    << ", line: " << codeLine;

                JsonObject expectedError;
                Errors::get( Errors::WRONG_PARAMS, expectedError );

                JsonObject error;
                ASSERT_TRUE( json_get_obj( switchModeAnswer, "error", error ) );
                ASSERT_EQ( json_get_str( error, "data" ), json_get_str( expectedError, "data" ) );
            }
            catch( std::exception const& e ) {
                Log::error( cc::error( "Exception: " ) + e.what() );
                FAIL() << "Exception occurred, line " << codeLine;
            }
        };

        auto answer = switchToMode( foundIt->m_name, __LINE__ );
        assertResults( answer, foundIt->m_name, foundIt->m_id, __LINE__, assertCallback );
    }
}

/* DONE 1.3 Try to switch to the house mode which doesn't match any existent one */
TEST_F( TestsHouseModeByName, HouseModeSwitchToNonexistent /* 1.3 */ ) {
    using HouseModesGetResult = CoreDRemoteInterface::CoreDInterface::HouseModesGetResult;

    HouseModesGetResult houseModes = remoteInterface.getHouseModes();
    std::string nonexistentName = "nonExistentHouseMode";
    std::string nonexistentId = "fake-id";

    auto foundIt = std::find_if(
        begin( houseModes ), end( houseModes ), [&]( HouseModesGetResult::value_type const& entry ) -> bool {
            return entry.m_name == nonexistentName;
        } );

    ASSERT_TRUE( foundIt == end( houseModes ) ) << "Name should be present in house mode names list";
    if( foundIt == end( houseModes ) ) {
        auto assertCallback = []( JsonObject const& switchModeAnswer,
                                  std::string const& modeName,
                                  std::string const& modeId,
                                  int codeLine ) {
            // some function
            try {
                ASSERT_TRUE( switchModeAnswer.is_object() && switchModeAnswer[ "error" ] != nullptr )
                    << ", line: " << codeLine;

                JsonObject error;
                ASSERT_TRUE( json_get_obj( switchModeAnswer, "error", error ) ) << ", line: " << codeLine;

                JsonObject errorExpected;
                Errors::get( Errors::HOUSE_MODE_NAME_DOES_NOT_EXIST, errorExpected );
                ASSERT_EQ( json_get_int( error, "code", 0 ), json_get_int( errorExpected, "code" ) )
                    << " expected: " << errorExpected.dump() << ", received: " << error.dump() << "\n";
                ASSERT_EQ( json_get_str( error, "data" ), json_get_str( errorExpected, "data" ) );
            }
            catch( std::exception const& e ) {
                Log::error( cc::error( "Exception: " ) + e.what() );
                FAIL() << "Exception occurred: '" << e.what() << "', line " << codeLine;
            }
        };

        auto answer = switchToMode( nonexistentName, __LINE__ );
        assertResults( answer, nonexistentName, nonexistentId, __LINE__, assertCallback );
    }
}

/* DONE 1.4 Try to switch using empty 'name' parameter. */
TEST_F( TestsHouseModeByName, HouseModeSwitchToEmptyName /* 1.4 */ ) {
    using HouseModesGetResult = CoreDRemoteInterface::CoreDInterface::HouseModesGetResult;

    HouseModesGetResult houseModes = remoteInterface.getHouseModes();
    std::string emptyName = "";
    std::string fakeId = "fake-id";

    auto assertCallback =
        []( JsonObject const& switchModeAnswer, std::string const& modeName, std::string const& modeId, int codeLine ) {
            try {
                ASSERT_TRUE( switchModeAnswer.is_object() && switchModeAnswer[ "error" ] != nullptr )
                    << ", line: " << codeLine;

                JsonObject error, data;
                ASSERT_TRUE( json_get_obj( switchModeAnswer, "error", error ) ) << ", line: " << codeLine;
                ASSERT_EQ( error[ "data" ].get< std::string >(), "rpc.params.invalid" ) << ", line: " << codeLine;
            }
            catch( std::exception const& e ) {
                Log::error( cc::error( "Exception: " ) + e.what() );
                FAIL() << "Exception occurred: '" << e.what() << "', line " << codeLine;
            }
        };

    auto answer = switchToMode( emptyName, __LINE__ );
    assertResults( answer, emptyName, fakeId, __LINE__, assertCallback );
}

}  // namespace Tests
}  // namespace HubPlatform