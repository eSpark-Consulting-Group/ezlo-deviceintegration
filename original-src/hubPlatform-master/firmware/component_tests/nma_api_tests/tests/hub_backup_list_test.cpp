#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. lua network/dhcp events subscriptions addon
DONE        1.0 dhcp subscribe
DONE        1.1 dhcp unsubscribe

*/

namespace HubPlatform {
namespace Tests {

class HubBackupListTest : public CoreNmaTest {
public:
    void SetUp() override;

    void TearDown() override;
};

void HubBackupListTest::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void HubBackupListTest::TearDown() {
    CoreNmaTest::TearDown();
}

TEST_F( HubBackupListTest, backupListGet ) {
    JsonObject firstFileData = JSON( "key"
                                     << "key_file_1"
                                     << "uuid"
                                     << "uuid_file_1"
                                     << "bucket"
                                     << "default"
                                     << "timestamp"
                                     << "123467542" );
    JsonObject secondFileData = JSON( "key"
                                      << "key_file_2"
                                      << "uuid"
                                      << "uuid_file_2"
                                      << "bucket"
                                      << "controller_backups"
                                      << "timestamp"
                                      << "1234154145" );
    nma().setCloudMethod( "cloud.storage_list", [&]( rpc::Message& rpcMessage ) {
        JsonObject& request = rpcMessage.getMessage();
        EXPECT_EQ( json_get_str( request, "method" ), "cloud.storage_list" );
        EXPECT_EQ( json_get_str( request, "api" ), "2" );
        JsonArray buckets;
        json_get_arr( request[ "params" ], "buckets", buckets );
        EXPECT_EQ( buckets[ 0 ], "default" );
        EXPECT_EQ( buckets[ 1 ], "controller_backups" );
        EXPECT_EQ( json_get_str( request[ "params" ], "type" ), "archive_controller_backup" );

        JsonObject& joAnswer = rpcMessage.getAnswer();
        joAnswer[ "method" ] = json_get_str( request, "method" );
        joAnswer[ "result" ] = JSON( "list" << JSON_ARRAY_( firstFileData, secondFileData ) );
        joAnswer[ "error" ] = nullptr;
        joAnswer[ "sender" ] = JSON( "type"
                                     << "controller"
                                     << "uuid"
                                     << "&0000410-uuid" );
    } );
    auto response = nma().sendRequest( GroupsNames::BACKUP, JSON_EMPTY_OBJ, BACKUP_LIST );
    JsonObject backups;
    json_get_arr( response, "backups", backups );
    ASSERT_TRUE( backups.is_array() );
    {
        auto backup = backups.at( 0 );
        ASSERT_TRUE( backup.is_object() );
        EXPECT_EQ( backup[ "bucket" ], firstFileData[ "bucket" ] );
        EXPECT_EQ( backup[ "key" ], firstFileData[ "key" ] );
        EXPECT_EQ( backup[ "uuid" ], firstFileData[ "uuid" ] );
    }
    {
        auto backup = backups.at( 1 );
        ASSERT_TRUE( backup.is_object() );
        EXPECT_EQ( backup[ "bucket" ], secondFileData[ "bucket" ] );
        EXPECT_EQ( backup[ "key" ], secondFileData[ "key" ] );
        EXPECT_EQ( backup[ "uuid" ], secondFileData[ "uuid" ] );
    }
}
}  // namespace Tests
}  // namespace HubPlatform