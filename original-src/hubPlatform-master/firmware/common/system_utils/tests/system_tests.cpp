#include "system_utils/system.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "database_client/dbtools.h"
#include "system_utils/filesystem.h"
#include <gtest/gtest.h>
#include <cstdlib>

/*

TESTPLAN

DONE    1. OS Memory Page Size
DONE    2. Get drive size by a folder name
DONE    3. Calculate lmdb database memory map storage by main memory size
DONE    4. Calculate lmdb database memory map storage by RAM memory size
DONE    5. getCommandOutput test
DONE        5.1 getCommandOutput test with real child process exit code
DONE    6. formatTimeoutOutput test

*/

namespace HubPlatform {
namespace UT {

class SystemTests : public ::testing::Test {};

TEST_F( SystemTests, osMemoryPageSize ) /* 1 */ {
    auto pageSize = System::getMemoryPageSize();
    Log::info( "System page size: " + cc::num10( pageSize ) );
    EXPECT_EQ( pageSize, 4096 );
}

TEST_F( SystemTests, mainStorageDataFolderMapSize ) /* 3 */ {
    uint64_t mapSize = HubPlatform::DatabaseClient::getStorageMapSizeDefault();
    Log::info( "Map size for data folder: " + cc::num10( mapSize ) );
    ASSERT_GT( mapSize, 0 );
}

TEST_F( SystemTests, tempStorageDataFolderMapSize ) /* 4 */ {
    uint64_t mapSize = HubPlatform::DatabaseClient::getTempStorageMapSizeDefault();
    Log::info( "Map size for data folder: " + cc::num10( mapSize ) );
    ASSERT_GT( mapSize, 0 );
}

TEST_F( SystemTests, getCommandOutput ) /* 5 */ {
    auto resultPair = System::getCommandOutput( "echo -n '111'", true, true );
    ASSERT_EQ( resultPair.first, "111" );
    ASSERT_EQ( resultPair.second, 0 );
}

TEST_F( SystemTests, getCommandOutputExitCode ) /* 5.1 */ {
    auto resultPair = System::getCommandOutput( "echo -n '111'; exit 2", true, true );
    ASSERT_EQ( resultPair.first, "111" );
    ASSERT_EQ( resultPair.second, 2 << 8 );

    auto unpacked = System::unpackExitCode( resultPair.second );
    ASSERT_EQ( unpacked.exitCode, 2 );
}

TEST_F( SystemTests, timeoutOutput ) /* 6 */ {
    std::string expectedCommand = "some-very-long-command";
    int expectedDuration = 123;
    auto command = System::formatTimeoutCommand( expectedCommand, expectedDuration );

    ASSERT_FALSE( command.empty() );

    using namespace std;
    istringstream iss( command );
    string s;
    vector< string > tokens;
    while( getline( iss, s, ' ' ) ) {
        tokens.push_back( s );
    }

    for( size_t i; i < tokens.size(); i++ ) {
        auto token = tokens[ i ];
        if( i == 0 ) {
            ASSERT_EQ( token, "timeout" ) << "Starts with 'timeout'";
        }
        else if( i == tokens.size() - 1 ) {
            ASSERT_EQ( token, expectedCommand ) << "Finishes with '" << expectedCommand << "'";
        }
        else {
            // options
            if( token.find( "--" ) == 0 ) {
                ASSERT_EQ( token, "--preserve-status" );
            }
            else if( token.find( '-' ) == 0 ) {
                ASSERT_EQ( token.size(), 2 ) << "Short options -s, -k";
                auto option = token[ 1 ];
                if( option == 's' ) {
                    // next is a signal number
                    i++;  // skip next token
                }
            }
            else {
                // check duration: floating.point number, followed by s, m, h, d (optional, default is 's')
                char* pEnd = nullptr;
                unsigned long duration = std::strtoul( token.c_str(), &pEnd, 10 );
                ASSERT_EQ( duration, expectedDuration ) << "Duration number";
                ASSERT_EQ( *pEnd, 's' ) << "Seconds prefix";
            }
        }
    }
}

}  // namespace UT
}  // namespace HubPlatform