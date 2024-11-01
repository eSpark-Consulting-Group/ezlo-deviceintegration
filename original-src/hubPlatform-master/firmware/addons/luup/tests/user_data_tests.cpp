#include "luup/user_data.h"

#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. User data initialization
DONE        1.1 Check data version is default initialized
DONE    2. Getting user data
DONE        2.1 Check empty result json when no changes
DONE        2.2 Check non empty result json when changes are
DONE        2.3 Check data version gets set into result json
DONE    3. Adding device
DONE        3.1 Check data version gets increased
DONE    4. Updating device
DONE        4.1 Check data version gets increased
DONE    5. Removing device
DONE        5.1 Check data version gets increased

*/

namespace HubPlatform {
namespace UT {

using namespace Luup;

class UserDataTestSuite : public testing::Test {
protected:
    UserDataTestSuite()
    : m_userData( m_db ) {}

    UserData& getUserData() {
        return m_userData;
    }

    const UserData& getUserData() const {
        return m_userData;
    }

    uint64_t getDataVersion() const {
        return getUserData().m_dataVersion;
    }

private:
    FakeDbClient m_db;
    UserData m_userData;
};

TEST_F( UserDataTestSuite, testCheckDataVersionIsDefaultInitialized /* 1.1 */ ) {
    EXPECT_EQ( getDataVersion(), 1 );
}

TEST_F( UserDataTestSuite, testCheckEmptyResultJsonWhenNoChanges /* 2.1 */ ) {
    auto result = getUserData().getData( 1 );
    EXPECT_EQ( result, JSON_EMPTY_OBJ );
}

TEST_F( UserDataTestSuite, testCheckNonEmptyResultJsonWhenChangesAre /* 2.2 */ ) {
    getUserData().addDevice();
    auto result = getUserData().getData();
    EXPECT_NE( result, JSON_EMPTY_OBJ );
}

TEST_F( UserDataTestSuite, testCheckDataVersionGetsSetIntoResultJson /* 2.3 */ ) {
    getUserData().addDevice();
    auto result = getUserData().getData();
    EXPECT_EQ( result, JSON( "DataVersion" << 2 ) );
}

TEST_F( UserDataTestSuite, testCheckDataVersionGetsIncreasedDuringDeviceAdding /* 3.1 */ ) {
    EXPECT_EQ( getDataVersion(), 1 );
    getUserData().addDevice();
    EXPECT_EQ( getDataVersion(), 2 );
}

TEST_F( UserDataTestSuite, testCheckDataVersionGetsIncreasedDuringDeviceUpdating /* 4.1 */ ) {
    EXPECT_EQ( getDataVersion(), 1 );
    getUserData().updateDevice( "" );
    EXPECT_EQ( getDataVersion(), 2 );
}

TEST_F( UserDataTestSuite, testCheckDataVersionGetsIncreasedDuringDeviceRemoving /* 5.1 */ ) {
    EXPECT_EQ( getDataVersion(), 1 );
    getUserData().removeDevice( "" );
    EXPECT_EQ( getDataVersion(), 2 );
}

}  // namespace UT
}  // namespace HubPlatform
