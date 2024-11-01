#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "system_utils/environment.h"

#include "logcmd/log_utils/log_utils.h"
#include "logcmd/log_exceptions.h"

#include "logcmd/tests/fakes/fake_filesystem.h"

/*

TESTPLAN

DONE    1. Search logs tests
DONE        1.0 Search log files
DONE        1.1 Search log files in empty dir
DONE        1.1 Search log files in non existing dir
DONE    2. Logs rotation tests
DONE        2.0 Rotate archives by num
DONE            2.0.0 Rotate archives by num
DONE            2.0.1 Rotate archives by num. Rotation not required
DONE            2.0.2 Rotate archives by num. No logs to rotate
DONE        2.1 Rotate archives by size
DONE            2.1.0 Rotate archives by size
DONE            2.1.1 Rotate archives by size. Rotation not required
DONE            2.1.2 Rotate archives by size. No logs to rotate
DONE        2.2 Rotate pending logs by size
DONE            2.2.0 Rotate pending logs by size
DONE            2.2.1 Rotate pending logs by size. Rotation not required
DONE            2.2.2 Rotate pending logs by size. No logs to rotate
DONE        2.3 Directory with logs does not exist
DONE    3. Log utils tests
DONE        3.0 Delete log files
DONE        3.1 Get logs size
DONE        3.2 Build partition map
DONE        3.3 Build partition map with date suffixes
DONE        3.4 Check rotated file names with date suffixes
*/

namespace HubPlatform {
namespace UT {

namespace {
using FileSet = std::set< std::string >;
}  // namespace

#define MB( value ) ( ( size_t )( value ) << 20 )

using namespace HubPlatform::LogCmd;

using testing::_;
using testing::Return;

class FileSearchTests : public ::testing::Test {
public:
    using FileSet = std::set< std::string >;

    std::set< std::string > set( const LogFiles& files ) {
        std::set< std::string > fileSet;
        for( auto& file : files )
            fileSet.emplace( file->fileName() );
        return fileSet;
    };
};

class LogsRotationTests : public ::testing::Test {};

class LogUtilsTests : public ::testing::Test {
public:
    using Files = std::vector< std::string >;
};

TEST_F( FileSearchTests, FindLogFiles /*1.0*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles(
        dirname,
        { "file1.log", "file1.log.1", "file2.log", "file2.log.2", "file3.logs.tgz", "nolog", "!invlid#NamE.log", "" } );
    filesystem.registerDirectory( dirname );

    EXPECT_EQ( set( LogUtils::findRegularLogs( filesystem, dirname ) ), FileSet( { "file1.log", "file2.log" } ) );
    EXPECT_EQ( set( LogUtils::findRotatedLogs( filesystem, dirname ) ), FileSet( { "file1.log.1", "file2.log.2" } ) );
    EXPECT_EQ( set( LogUtils::findSelectedLogs( filesystem, dirname, { "file2.log" } ) ), FileSet( { "file2.log" } ) );
    EXPECT_EQ( set( LogUtils::findLogs( filesystem, dirname ) ),
               FileSet( { "file1.log", "file2.log", "file1.log.1", "file2.log.2" } ) );
    EXPECT_EQ( set( LogUtils::findArchivedLogs( filesystem, dirname ) ), FileSet( { "file3.logs.tgz" } ) );
}

TEST_F( FileSearchTests, FindLogFilesInEmptyDir /*1.1*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, {} );
    filesystem.registerDirectory( dirname );

    EXPECT_EQ( set( LogUtils::findRegularLogs( filesystem, dirname ) ), FileSet() );
    EXPECT_EQ( set( LogUtils::findRotatedLogs( filesystem, dirname ) ), FileSet() );
    EXPECT_EQ( set( LogUtils::findSelectedLogs( filesystem, dirname, { "file2.log" } ) ), FileSet() );
    EXPECT_EQ( set( LogUtils::findLogs( filesystem, dirname ) ), FileSet() );
    EXPECT_EQ( set( LogUtils::findArchivedLogs( filesystem, dirname ) ), FileSet() );
}

TEST_F( FileSearchTests, FindLogFilesInNonExistingDir /*1.2*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";

    EXPECT_THROW( LogUtils::findRegularLogs( filesystem, dirname ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::findRotatedLogs( filesystem, dirname ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::findSelectedLogs( filesystem, dirname, { "file2.log" } ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::findLogs( filesystem, dirname ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::findArchivedLogs( filesystem, dirname ), LogDirectoryDoesNotExistError );
}

TEST_F( LogsRotationTests, rotateArchivesByNum /*2.0.0*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, { "a_logs.tgz", "b_logs.tgz", "c_logs.tgz", "d_logs.tgz", "not_log_file", "" } );
    filesystem.registerDirectory( dirname );

    LogUtils::rotateArchivesByNum( filesystem, dirname, 2 );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet( { "/dirname/a_logs.tgz", "/dirname/b_logs.tgz" } ) );
}

TEST_F( LogsRotationTests, rotateArchivesByNumNotRequired /*2.0.1*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, { "a_logs.tgz", "b_logs.tgz", "c_logs.tgz", "not_log_file", "" } );
    filesystem.registerDirectory( dirname );

    LogUtils::rotateArchivesByNum( filesystem, dirname, 5 );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, NoLogArchivesToRotateByNum /*2.0.2*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, { "not_log_file", "" } );
    filesystem.registerDirectory( dirname );

    LogUtils::rotateArchivesByNum( filesystem, dirname, 5 );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, RotateArchivesBySize /*2.1.0*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = {
        "a_logs.tgz", "b_logs.tgz", "c_logs.tgz", "d_logs.tgz", "not_log_file", ""
    };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    for( auto& file : filenames )
        filesystem.m_fileSizeMap[ dirname + "/" + file ] = MB( 10 );

    LogUtils::rotateArchivesBySize( filesystem, dirname, MB( 20 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet( { "/dirname/a_logs.tgz", "/dirname/b_logs.tgz" } ) );
}

TEST_F( LogsRotationTests, RotateArchivesBySizeNotRequired /*2.1.1*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = { "a_logs.tgz", "b_logs.tgz", "c_logs.tgz", "not_log_file", "" };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    for( auto& file : filenames )
        filesystem.m_fileSizeMap[ dirname + "/" + file ] = MB( 10 );

    LogUtils::rotateArchivesBySize( filesystem, dirname, MB( 500 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, NoLogArchivesToRotateBySize /*2.1.2*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, { "not_log_file", "" } );
    filesystem.registerDirectory( dirname );

    LogUtils::rotateArchivesBySize( filesystem, dirname, MB( 10 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, RotatePendingLogsBySize /*2.2.0*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = {
        "file1.log.1", "file1.log.2", "file2.log.1", "file2.log.2", "file3.log", "not_log_file", ""
    };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    for( auto& file : filenames )
        filesystem.m_fileSizeMap[ dirname + "/" + file ] = MB( 10 );

    LogUtils::rotatePendingLogsBySize( filesystem, dirname, MB( 25 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet( { "/dirname/file1.log.2", "/dirname/file2.log.2" } ) );
}

TEST_F( LogsRotationTests, RotatePendingLogsBySizeNotRequired /*2.2.1*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = {
        "file1.log.1", "file1.log.2", "file2.log.1", "file2.log.2", "file3.log", "not_log_file", ""
    };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    for( auto& file : filenames )
        filesystem.m_fileSizeMap[ dirname + "/" + file ] = MB( 10 );

    LogUtils::rotatePendingLogsBySize( filesystem, dirname, MB( 500 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, NoPendingLogsToRotateBySize /*2.2.2*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    filesystem.registerFiles( dirname, { "not_log_file", "" } );
    filesystem.registerDirectory( dirname );

    LogUtils::rotatePendingLogsBySize( filesystem, dirname, MB( 10 ) );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet() );
}

TEST_F( LogsRotationTests, DirectoryWithLogsDoesNotExist /*2.3*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";

    EXPECT_THROW( LogUtils::rotateArchivesByNum( filesystem, dirname, 10 ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::rotateArchivesBySize( filesystem, dirname, 10 ), LogDirectoryDoesNotExistError );
    EXPECT_THROW( LogUtils::rotatePendingLogsBySize( filesystem, dirname, 10 ), LogDirectoryDoesNotExistError );
}

TEST_F( LogUtilsTests, DeleteLogFiles /*3.0*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = { "file1.log", "file2.log", "file3.log" };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    LogFiles logFiles;
    logFiles.emplace_back( LogFile::makeLogFile( dirname, "file1.log" ) );
    logFiles.emplace_back( LogFile::makeLogFile( dirname, "file2.log" ) );

    LogUtils::deleteLogFiles( filesystem, logFiles );
    EXPECT_EQ( filesystem.m_removedFilesSet, FileSet( { "/dirname/file1.log", "/dirname/file2.log" } ) );
}

TEST_F( LogUtilsTests, GetLogsSize /*3.1*/ ) {
    FakeFilesystem filesystem;
    const std::string dirname = "/dirname";
    std::vector< std::string > filenames = { "file1.log", "file2.log", "file3.log" };
    filesystem.registerFiles( dirname, filenames );
    filesystem.registerDirectory( dirname );

    for( auto& file : filenames )
        filesystem.m_fileSizeMap[ dirname + "/" + file ] = MB( 10 );

    LogFiles logFiles;
    logFiles.emplace_back( LogFile::makeLogFile( dirname, "file1.log" ) );
    logFiles.emplace_back( LogFile::makeLogFile( dirname, "file2.log" ) );

    EXPECT_EQ( LogUtils::getLogsSize( filesystem, logFiles ), MB( 20 ) );
}

TEST_F( LogUtilsTests, BuildPartitionMap /*3.2*/ ) {
    auto file = []( const std::string& filename ) { return LogFile::makeLogFile( "/", filename ); };
    const LogFiles files{
        file( "file1.log.1" ), file( "file2.log.1" ), file( "file2.log.2" ), file( ".log.1" ), file( "" )
    };

    auto map = LogUtils::buildPartitionMap( files );
    EXPECT_EQ( map.size(), 2 );
    ASSERT_EQ( map.count( "file1.log" ), 1 );
    EXPECT_EQ( map[ "file1.log" ], 1 );

    ASSERT_EQ( map.count( "file2.log" ), 1 );
    EXPECT_EQ( map[ "file2.log" ], 2 );
}

TEST_F( LogUtilsTests, BuildPartitionMapByDate /*3.3*/ ) {
    auto file = []( const std::string& filename ) { return LogFile::makeLogFile( "/", filename ); };
    const LogFiles files{ file( "file1.log.0505123500" ),
                          file( "file2.log.0505123500" ),
                          file( "file2.log.0505124000" ),
                          file( ".log.0505123500" ),
                          file( "" ) };

    auto map = LogUtils::buildPartitionMap( files );
    EXPECT_EQ( map.size(), 2 );
    ASSERT_EQ( map.count( "file1.log" ), 1 );
    EXPECT_EQ( map[ "file1.log" ], 505123500 );

    ASSERT_EQ( map.count( "file2.log" ), 1 );
    EXPECT_EQ( map[ "file2.log" ], 505124000 );
}

TEST_F( LogUtilsTests, checkRotatedFileName /*3.4*/ ) {
    auto file = []( const std::string& filename ) { return LogFile::makeLogFile( "/", filename ); };

    auto rotatedFileName = LogFile::makeRotatedLogFileName( "file1", "log", "0505123400" );
    ASSERT_TRUE( LogFile::isRotatedLogFile( rotatedFileName ) );
    std::shared_ptr< LogFile > logFile;

    ASSERT_NO_THROW( logFile = file( rotatedFileName ) );
    EXPECT_TRUE( logFile != nullptr );
    EXPECT_TRUE( logFile->hasDateSuffix() );

    rotatedFileName = LogFile::makeRotatedLogFileName( "file1", "log", "2" );
    ASSERT_TRUE( LogFile::isRotatedLogFile( rotatedFileName ) );
    ASSERT_NO_THROW( logFile = file( rotatedFileName ) );
    EXPECT_TRUE( logFile != nullptr );
    EXPECT_FALSE( logFile->hasDateSuffix() );

    rotatedFileName = LogFile::makeRotatedLogFileName( "file1", "log", "notADigit" );
    ASSERT_FALSE( LogFile::isRotatedLogFile( rotatedFileName ) );
    ASSERT_NO_THROW( logFile = file( rotatedFileName ) );
    EXPECT_TRUE( logFile == nullptr );
}

}  // namespace UT
}  // namespace HubPlatform
