#include "networkd/gsm/nm_gsm_modem_active_simcards_table.h"

#include "database_client/fake_db/fake_db_client.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Network {
namespace UT {

/*

TESTPLAN

DONE        1.1 Test table API

*/

class GSMModemActiveSimcardsTableTestSuite : public ::testing::Test {};

TEST( GSMModemActiveSimcardsTableTestSuite, testTableAPI ) /*1.1*/ {
    HubPlatform::UT::FakeDbClient dbClient;
    GSMModemActiveSimcardsTable table( dbClient );

    // Read from empty table
    EXPECT_TRUE( table.getICCID( "unknown-imei" ).empty() );

    // Store simcard
    table.storeSimcard( "imei-1", "iccid-1" );
    EXPECT_EQ( table.getICCID( "imei-1" ), "iccid-1" );

    // Store second simcard
    table.storeSimcard( "imei-2", "iccid-2" );
    EXPECT_EQ( table.getICCID( "imei-1" ), "iccid-1" );
    EXPECT_EQ( table.getICCID( "imei-2" ), "iccid-2" );

    // Override second simcard
    table.storeSimcard( "imei-2", "iccid-2.2" );
    EXPECT_EQ( table.getICCID( "imei-2" ), "iccid-2.2" );

    // Clear second simcard
    table.storeSimcard( "imei-2", "" );
    EXPECT_TRUE( table.getICCID( "imei-2" ).empty() );
}

}  // namespace UT
}  // namespace Network
}  // namespace HubPlatform
