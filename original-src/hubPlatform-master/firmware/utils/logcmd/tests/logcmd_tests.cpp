//#include "test_utils/temporary_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common_utils/cpp14.h"
#include "system_utils/environment.h"
#include "system_utils/crontab/crontab.h"

#include "logcmd/log_filter.h"
#include "logcmd/log_common.h"
#include "logcmd/log_arg_parse.h"
#include "logcmd/log_storage.h"
#include "logcmd/log_storage_impl.h"
#include "logcmd/log_exceptions.h"

/*

TESTPLAN

DONE    1. LogFile tests
DONE        1.0 Create log files
DONE    2. LogArgParse tests
DONE        2.0 Default args
DONE        2.1 Non default args
DONE        2.2 Store to postponed args
DONE        2.3 Upload postponed args
DONE        2.4 Invalid args
DONE    3. LogStorage tests
DONE        3.0 Successful upload
DONE        3.1 Failed upload
DONE    4. LogCronSchedule tests
DONE        4.0 Generate cron schedule
*/

namespace HubPlatform {
namespace UT {

#define MB( value ) ( ( size_t )( value ) << 20 )

using namespace HubPlatform::LogCmd;

using testing::_;
using testing::Return;

class FakeLogStorage : public LogStorageImpl {
public:
    ~FakeLogStorage() override = default;
    FakeLogStorage( std::string error, Common::CloudStorageInterface::FileKey fileKey )
    : m_error( std::move( error ) )
    , m_fileKey( std::move( fileKey ) ) {}
    void upload( const std::string& filePath, const Json& storageMetadata, UploadResponse response ) override {
        response( {}, m_error, m_fileKey );
    }
    void stopTransfer() override{};

private:
    const std::string m_error;
    const Common::CloudStorageInterface::FileKey m_fileKey;
};

class LogFileTest : public ::testing::Test {};

class LogArgParseTest : public ::testing::Test {};

class LogStorageTest : public ::testing::Test {};

class LogCronScheduleTest : public ::testing::Test {};

TEST_F( LogFileTest, CreateLogFile /*1.0*/ ) {
    EXPECT_FALSE( LogFile::makeLogFile( "/", "" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", ".log" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", ".log.1" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file.x" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file/name.log" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "/" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file*.log" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "*.log" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file`|+,().log" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "logs.tgz" ) );
    EXPECT_FALSE( LogFile::makeLogFile( "/", "file.logs.1.tgz" ) );

    {  // regular log file 1
        auto logFile = LogFile::makeLogFile( "/dirname", "file.log" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Regular );
        EXPECT_EQ( logFile->path(), "/dirname/file.log" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "file.log" );
        EXPECT_EQ( logFile->logName(), "file.log" );
        EXPECT_EQ( logFile->partNum(), 0 );
        EXPECT_EQ( logFile->hasDateSuffix(), false );
    }

    {  // regular log file 2
        auto logFile = LogFile::makeLogFile( "/dirname", "ha-file_name.log" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Regular );
        EXPECT_EQ( logFile->path(), "/dirname/ha-file_name.log" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "ha-file_name.log" );
        EXPECT_EQ( logFile->logName(), "ha-file_name.log" );
        EXPECT_EQ( logFile->partNum(), 0 );
        EXPECT_EQ( logFile->hasDateSuffix(), false );
    }

    {  // rotated log file 1
        auto logFile = LogFile::makeLogFile( "/dirname", "file.log.1" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Rotated );
        EXPECT_EQ( logFile->path(), "/dirname/file.log.1" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "file.log.1" );
        EXPECT_EQ( logFile->logName(), "file.log" );
        EXPECT_EQ( logFile->partNum(), 1 );
        EXPECT_EQ( logFile->hasDateSuffix(), false );
    }

    {  // rotated log file 2
        auto logFile = LogFile::makeLogFile( "/dirname", "ha-file_name.log.10" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Rotated );
        EXPECT_EQ( logFile->path(), "/dirname/ha-file_name.log.10" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "ha-file_name.log.10" );
        EXPECT_EQ( logFile->logName(), "ha-file_name.log" );
        EXPECT_EQ( logFile->partNum(), 10 );
        EXPECT_EQ( logFile->hasDateSuffix(), false );
    }

    {  // rotated log file 3: with date suffix
        auto logFile = LogFile::makeLogFile( "/dirname", "ha-file_name.log.0505235900" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Rotated );
        EXPECT_EQ( logFile->path(), "/dirname/ha-file_name.log.0505235900" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "ha-file_name.log.0505235900" );
        EXPECT_EQ( logFile->logName(), "ha-file_name.log" );
        EXPECT_EQ( logFile->partNum(), 505235900 );
        EXPECT_EQ( logFile->hasDateSuffix(), true );
    }

    {  // archive log file 1
        auto logFile = LogFile::makeLogFile( "/dirname", "file_logs.tgz" );
        ASSERT_TRUE( logFile );
        EXPECT_EQ( logFile->type(), LogFile::LogType::Archive );
        EXPECT_EQ( logFile->path(), "/dirname/file_logs.tgz" );
        EXPECT_EQ( logFile->dirName(), "/dirname" );
        EXPECT_EQ( logFile->fileName(), "file_logs.tgz" );
        EXPECT_THROW( logFile->logName(), LogFileError );
        EXPECT_EQ( logFile->partNum(), 0 );
        EXPECT_EQ( logFile->hasDateSuffix(), false );
    }
}

TEST_F( LogArgParseTest, DefaultArgs /*2.0*/ ) {
    Args args;
    auto j = parseArgs( args );

    EXPECT_EQ( json_get_str( j, ARG_LOG_DIR ), Environment::getLogsFolder() );
    EXPECT_EQ( json_get_str( j, ARG_OUTPUT_DIR ), "/tmp" );

    EXPECT_FALSE( json_contains( j, ARG_ROTATED_LOGS ) );
    EXPECT_FALSE( json_contains( j, ARG_SELECTED_LOGS ) );
    EXPECT_FALSE( json_contains( j, ARG_UPLOAD ) );
    EXPECT_FALSE( json_contains( j, ARG_AUTOREMOVE ) );
}

TEST_F( LogArgParseTest, NonDefaultArgs /*2.1*/ ) {
    Args args{
        "--select=fileA.log,fileB.log",
        "--log-dir=/path/to/logdir",
        "--output-dir=/output-dir/",
        "--create-dir",
        "--store",
        "--autoremove",
        "--rotate-archives=5",
        "--idle",
    };

    auto j = parseArgs( args );

    EXPECT_EQ( json_get_str( j, ARG_LOG_DIR ), "/path/to/logdir" );
    EXPECT_EQ( json_get_str( j, ARG_OUTPUT_DIR ), "/output-dir" ) << "NOTE: trailing '/' should be removed";
    EXPECT_TRUE( json_contains( j, ARG_CREATE_DIR ) );
    EXPECT_TRUE( json_contains( j, ARG_SELECTED_LOGS ) );
    EXPECT_TRUE( json_contains( j, ARG_STORE ) );
    EXPECT_TRUE( json_contains( j, ARG_AUTOREMOVE ) );
    EXPECT_TRUE( json_contains( j, ARG_IDLE ) );
    EXPECT_EQ( json_get_int( j, ARG_ROTATE_ARCHIVES ), 5 );

    auto files = json_get_vec< std::string >( j, ARG_SELECTED_LOGS );
    EXPECT_EQ( files.size(), 2 );
    EXPECT_EQ( files[ 0 ], "fileA.log" );
    EXPECT_EQ( files[ 1 ], "fileB.log" );

    EXPECT_FALSE( json_contains( j, ARG_ALL_LOGS ) );
    EXPECT_FALSE( json_contains( j, ARG_ROTATED_LOGS ) );
}

TEST_F( LogArgParseTest, StoreToPostponedArgs /*2.2*/ ) {
    Args args{
        ARG_STORE_TO_POSTPONED,
    };

    auto j = parseArgs( args );

    EXPECT_TRUE( json_contains( j, ARG_STORE_TO_POSTPONED ) );
    EXPECT_TRUE( json_contains( j, ARG_IDLE ) );
    EXPECT_TRUE( json_contains( j, ARG_CREATE_DIR ) );
    EXPECT_EQ( json_get_str( j, ARG_OUTPUT_DIR ), Environment::getPostponedLogsFolder() );
}

TEST_F( LogArgParseTest, UploadPostponedArgs /*2.3*/ ) {
    Args args{
        ARG_UPLOAD_POSTPONED,
    };

    auto j = parseArgs( args );

    EXPECT_TRUE( json_contains( j, ARG_UPLOAD_POSTPONED ) );
    EXPECT_TRUE( json_contains( j, ARG_IDLE ) );
    EXPECT_TRUE( json_contains( j, ARG_AUTOREMOVE ) );
    EXPECT_EQ( json_get_str( j, ARG_LOG_DIR ), Environment::getPostponedLogsFolder() );
}

TEST_F( LogArgParseTest, InvalidArgs /*2.4*/ ) {
    EXPECT_THROW( parseArgs( { "--upload=true" } ), LogArgError ) << "Flag argument should not contain value";

    EXPECT_THROW( parseArgs( { "--output-dir=" } ), LogArgError ) << "Empty output directory";
    EXPECT_THROW( parseArgs( { "--output-dir=not-abs" } ), LogArgError ) << "Not absolute path";
    EXPECT_NO_THROW( parseArgs( { "--output-dir=/path/to/dir" } ) );

    EXPECT_THROW( parseArgs( { "--output-name=" } ), LogArgError ) << "Empty archive name";
    EXPECT_THROW( parseArgs( { "--output-name=/path/foo.tgz" } ), LogArgError ) << "Path included";
    EXPECT_THROW( parseArgs( { "--output-name=foo.zip" } ), LogArgError ) << "Wrong extension";
    EXPECT_THROW( parseArgs( { "--output-name=*.tgz" } ), LogArgError ) << "Invalid name";
    EXPECT_NO_THROW( parseArgs( { "--output-name=foo.tgz" } ) );

    EXPECT_THROW( parseArgs( { "--rotate-archives=" } ), LogArgError ) << "Empty rotation value";
    EXPECT_THROW( parseArgs( { "--rotate-archives=x" } ), LogArgError ) << "Rotation value is not int";
    EXPECT_THROW( parseArgs( { "--rotate-archives=0" } ), LogArgError ) << "Rotation value should be greater than 0";
    EXPECT_THROW( parseArgs( { "--rotate-archives=-1" } ), LogArgError ) << "Rotation value should be greater than 0";
    EXPECT_NO_THROW( parseArgs( { "--rotate-archives=5" } ) );

    EXPECT_THROW( parseArgs( { "--upload", "--store" } ), LogArgError ) << "Mutually exclusive flags";
    EXPECT_THROW( parseArgs( { "--all", "--rotated" } ), LogArgError ) << "Mutually exclusive flags";

    EXPECT_THROW( parseArgs( { "--rotate", "--rotate-on-size-limit" } ), LogArgError ) << "Mutually exclusive commands";
    EXPECT_THROW( parseArgs( { "--slice", "--rotate-on-size-limit" } ), LogArgError ) << "Mutually exclusive commands";
}

TEST_F( LogArgParseTest, RotateOnSizeLimitArgs /*2.5*/ ) {
    Args args{
        ARG_ROTATE_ON_SIZE_LIMIT,
    };

    auto j = parseArgs( args );

    EXPECT_TRUE( json_contains( j, ARG_ROTATE_ON_SIZE_LIMIT ) );
}

TEST_F( LogStorageTest, LogStorageUploadSuccess /*3.0*/ ) {
    auto fileKey = Common::CloudStorageInterface::FileKey( "1234", "abcd", "" );
    auto fakeLogStorage = std::make_unique< FakeLogStorage >( "", fileKey );

    LogStorage storage( std::move( fakeLogStorage ) );
    auto result = storage.upload( "/path/to/logs.tgz", JSON_EMPTY_OBJ );
    EXPECT_EQ( json_get_str( result, "error" ), "" );
    EXPECT_EQ( result[ "fileStorageInfo" ], fileKey.toJson() );
}

TEST_F( LogStorageTest, LogStorageUploadFailure /*3.1*/ ) {
    auto fileKey = Common::CloudStorageInterface::FileKey( "1234", "abcd", "" );
    auto fakeLogStorage = std::make_unique< FakeLogStorage >( "dummyError", fileKey );

    LogStorage storage( std::move( fakeLogStorage ) );
    EXPECT_THROW( storage.upload( "/logs.tgz", JSON_EMPTY_OBJ ), LogUploadError ) << "Invalid response";
}

TEST_F( LogCronScheduleTest, GenerateCronSchedule /*4.0*/ ) {
    using Schedule = system_utils::Crontab::Schedule;
    auto getSchedule = [&]( const std::string& minutes, const std::string& hours ) {
        Schedule schedule;
        schedule.fill( "*" );
        schedule[ 0 ] = minutes;
        schedule[ 1 ] = hours;
        return schedule;
    };

    EXPECT_EQ( generateCronSchedule( 1, 0 ), getSchedule( "0", "*/1" ) );
    EXPECT_EQ( generateCronSchedule( 1, 7 ), getSchedule( "7", "*/1" ) );

    EXPECT_EQ( generateCronSchedule( 8, 0 ), getSchedule( "0", "0,8,16" ) );
    EXPECT_EQ( generateCronSchedule( 8, 1 ), getSchedule( "1", "1,9,17" ) );

    EXPECT_EQ( generateCronSchedule( 12, 0 ), getSchedule( "0", "0,12" ) );
    EXPECT_EQ( generateCronSchedule( 12, 24003 ), getSchedule( "3", "3,15" ) );
    EXPECT_EQ( generateCronSchedule( 12, 92000375 ), getSchedule( "35", "11,23" ) );

    // intervals > 12 must be triggered once per day
    EXPECT_EQ( generateCronSchedule( 16, 0 ), getSchedule( "0", "16" ) );
    EXPECT_EQ( generateCronSchedule( 16, 1 ), getSchedule( "1", "17" ) );
    EXPECT_EQ( generateCronSchedule( 24, 7 ), getSchedule( "7", "7" ) );
    EXPECT_EQ( generateCronSchedule( 24 + 5, 0 ), getSchedule( "0", "5" ) );

    EXPECT_EQ( generateCronSchedule( 0, 7 ), getSchedule( "7", "*/1" ) );
}

}  // namespace UT
}  // namespace HubPlatform
