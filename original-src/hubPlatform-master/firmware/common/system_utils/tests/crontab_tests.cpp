
#include "system_utils/crontab/crontab.h"
#include "system_utils/crontab/crontab_utils.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <fstream>
#include <array>

/*

TESTPLAN

DONE    1. Parse crontab record
DONE    2. Add crontab record
DONE    3. Delete crontab record
DONE    4. Find crontab record
DONE    5. Update command schedule
DONE    6. Load crontab file
DONE    7. Write crontab file

*/

using namespace ::testing;

namespace HubPlatform {
namespace UT {

using namespace system_utils;

class CrontabTestSuite : public Test {
public:
    void createCrontabFile( const std::string& content ) {
        filesystem::createFolder( getCrontabDirPath() );
        std::ofstream file( getCrontabFullPath() );
        file << content;
    }

    std::string getCrontabFileContent() {
        std::ifstream systemConfig( getCrontabFullPath() );
        std::string content( ( std::istreambuf_iterator< char >( systemConfig ) ), std::istreambuf_iterator< char >() );
        return content;
    }

    std::string getCrontabFullPath() {
        return getCrontabDirPath() + "/root";
    }

    std::list< Crontab::Record >& getRecords( Crontab& crontab ) {
        return crontab.m_records;
    }

private:
    std::string getCrontabDirPath() {
        return m_tmpDir.getPath() + "/etc/crontabs";
    }

    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

TEST_F( CrontabTestSuite, parseCrontabLine /*1*/ ) {
    auto record = crontab_utils::parseLine( "0 1 2 3 4 command arg1\n" );
    EXPECT_TRUE( record );

    EXPECT_EQ( record->first, Crontab::Schedule( { "0", "1", "2", "3", "4" } ) );
    EXPECT_EQ( record->second, "command arg1" );

    EXPECT_FALSE( crontab_utils::parseLine( "" ) ) << "Empty line";
    EXPECT_FALSE( crontab_utils::parseLine( "  " ) ) << "Empty line";
    EXPECT_FALSE( crontab_utils::parseLine( "\n" ) ) << "Empty line";
    EXPECT_FALSE( crontab_utils::parseLine( "  \n " ) ) << "Empty line";

    EXPECT_FALSE( crontab_utils::parseLine( "# comment line" ) );

    EXPECT_THROW( crontab_utils::parseLine( "0 1 command" ), CrontabInvalidFileError ) << "Invalid schedule";
    EXPECT_THROW( crontab_utils::parseLine( "0 1 2 3 4 " ), CrontabInvalidFileError ) << "Empty command";
}

TEST_F( CrontabTestSuite, addRecord /*2*/ ) {
    Crontab crontab;

    const Crontab::Record record1{ { "0", "1", "2", "3", "4" }, "command1 arg1 arg2" };
    const Crontab::Record record2{ { "a", "b", "c", "d", "e" }, "command2 argA argB" };

    crontab.addRecord( record1 );
    crontab.addRecord( record2 );

    auto& records = getRecords( crontab );
    EXPECT_EQ( records.size(), 2 );
    EXPECT_TRUE( crontab.isUpdated() );

    EXPECT_EQ( records.front(), record1 );
    EXPECT_EQ( records.back(), record2 );
}

TEST_F( CrontabTestSuite, deleteRecord /*3*/ ) {
    Crontab crontab;

    const Crontab::Record record1{ { "0", "1", "2", "3", "4" }, "command1 arg1 arg2" };
    const Crontab::Record record2{ { "0", "1", "2", "3", "4" }, "command2 arg1 arg2" };
    const Crontab::Record record3{ { "0", "1", "2", "3", "4" }, "command3 arg1 arg2" };
    crontab.addRecord( record1 );
    crontab.addRecord( record2 );
    crontab.addRecord( record3 );

    crontab.deleteRecord( "command2" );

    auto& records = getRecords( crontab );
    EXPECT_EQ( records.size(), 2 );
    EXPECT_TRUE( crontab.isUpdated() );

    EXPECT_EQ( records.front(), record1 );
    EXPECT_EQ( records.back(), record3 );

    EXPECT_THROW( crontab.deleteRecord( "unknownCommand" ), CrontabRecordError );
}

TEST_F( CrontabTestSuite, findRecord /*4*/ ) {
    Crontab crontab;

    const Crontab::Record record1{ { "0", "1", "2", "3", "4" }, "command1 arg1 arg2" };
    const Crontab::Record record2{ { "a", "b", "c", "d", "e" }, "command2 argA argB" };
    crontab.addRecord( record1 );
    crontab.addRecord( record2 );

    auto record = crontab.findRecord( "command2" );
    ASSERT_TRUE( record );
    EXPECT_EQ( *record, record2 );

    EXPECT_FALSE( crontab.findRecord( "unknownCommand" ) );
}

TEST_F( CrontabTestSuite, updateCommandSchedule /*5*/ ) {
    Crontab crontab;
    crontab.addRecord( { { "0", "1", "2", "3", "4" }, "command arg1 arg2" } );

    const Crontab::Record expectedRecord{ { "a", "b", "c", "d", "e" }, "command arg1 arg2" };
    crontab.updateCommandSchedule( "command", expectedRecord.first );

    auto& records = getRecords( crontab );
    ASSERT_EQ( records.size(), 1 );
    EXPECT_EQ( records.front(), expectedRecord );

    EXPECT_THROW( crontab.updateCommandSchedule( "unknownCommand", expectedRecord.first ), CrontabRecordError );
}

TEST_F( CrontabTestSuite, loadFile /*6*/ ) {
    createCrontabFile( "#comment line\n"
                       "0 1 2 3 4 command1 arg1 arg2\n"
                       "\n"
                       "a b c d e command2 arg1 arg2\n" );

    Crontab crontab( getCrontabFullPath() );
    EXPECT_NO_THROW( crontab.load() );

    const Crontab::Record expectedRecord1{ { "0", "1", "2", "3", "4" }, "command1 arg1 arg2" };
    const Crontab::Record expectedRecord2{ { "a", "b", "c", "d", "e" }, "command2 arg1 arg2" };

    auto& records = getRecords( crontab );
    ASSERT_EQ( records.size(), 2 );
    EXPECT_EQ( records.front(), expectedRecord1 );
    EXPECT_EQ( records.back(), expectedRecord2 );
}

TEST_F( CrontabTestSuite, write /*7*/ ) {
    createCrontabFile( "0 1 2 3 4 command1 arg1 arg2" );

    const Crontab::Record expectedRecord1{ { "0", "1", "2", "3", "4" }, "command1 arg1 arg2" };
    const Crontab::Record expectedRecord2{ { "a", "b", "c", "d", "e" }, "command2 arg1 arg2" };

    Crontab crontab( getCrontabFullPath() );
    EXPECT_NO_THROW( crontab.load() );

    crontab.addRecord( expectedRecord2 );
    EXPECT_NO_THROW( crontab.write() );

    Crontab updatedCrontab( getCrontabFullPath() );
    EXPECT_NO_THROW( updatedCrontab.load() );

    auto& records = getRecords( updatedCrontab );
    ASSERT_EQ( records.size(), 2 );
    EXPECT_EQ( records.front(), expectedRecord1 );
    EXPECT_EQ( records.back(), expectedRecord2 );
}

}  // namespace UT
}  // namespace HubPlatform
