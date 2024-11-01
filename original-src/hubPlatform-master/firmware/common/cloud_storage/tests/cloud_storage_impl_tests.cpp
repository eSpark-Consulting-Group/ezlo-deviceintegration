#include "cloud_storage/cloud_storage_impl.h"
#include "cloud_storage/resources.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "common_utils/curl_wrapper.h"
#include "json_utils/json_utils.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include "system_utils/process/forker.h"
#include "system_utils/environment.h"
#include "test_utils/test_environment.h"
#include "base_utils/utils.h"
#include "system_utils/filesystem.h"
#include "../../../addons/cameras/defs.h"
#include <thread>
#include <future>
#include <fstream>

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Common {
namespace UT {

/*
TEST PLAN

    1. Request options  validation
        1.1 Invalid token error
        1.2 Invalid Xapp error
    2. Upload with redirect
        2.1 Upload with redirect, successful
    3. Download with redirect
        2.1 Download with redirect in one step, successful
        2.2 Download with redirect, 404 http status, download without redirect
    4. Upload without redirect
    5. Different Buckets selection by StorageType.

 */

class CloudStorageImplTests : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
    IDbClient& getDBClient();
    static void SetUpTestCase();
    static void TearDownTestCase();

    static int& getMockServerPort();

    static void startTestMockServer( unsigned port );

    static pid_t& mockServerPid();
    static bool& initializationComplete();
    void checkInitializationComplete( int codeLine );
    void callingFile( std::string file );

protected:
    static std::string makeHostPortUrl( const std::string& path,
                                        int port,
                                        const std::string& host = "127.0.0.1",
                                        const std::string& protocol = "http" );
    static JsonObject makeCloudStorageRecordLocal( int port, const std::string& xapp = "", const Json& prefixes = {} );
    static void initControllerToken( IDbClient& dbClient, const JsonObject& decryptedTokenRecord, int codeLine );
    static bool makePingRequest();
    static std::string makeTempFileName();

    CloudStorageInterface::StorageBucket getBucket( CloudStorageInterface::StorageType storageType ) {
        return CloudStorageInterface::CloudStorageImpl::getBucket( storageType );
    }

protected:
    std::unique_ptr< IDbClient > m_dbClient;
    std::string m_callingFile;
};

void CloudStorageImplTests::SetUp() {
    m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
    callingFile( __FILE__ );
}

void CloudStorageImplTests::TearDown() {
    m_dbClient.reset();
}

IDbClient& CloudStorageImplTests::getDBClient() {
    return *m_dbClient;
}

void CloudStorageImplTests::SetUpTestCase() {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    LOGGER_TRACE( "getUniqueFreeLocalPort CloudStorageImplTests pending..." );
    {
        Process::SharedMutex processMutex( "uniquePortMutex" );
        std::lock_guard< Process::SharedMutex > lock( processMutex );

        LOGGER_DEBUG( "getUniqueFreeLocalPort cloud_storage_impl_tests" );

        getMockServerPort() = system_utils::getUniqueFreeLocalPort();
    }
    LOGGER_TRACE( "getUniqueFreeLocalPort CloudStorageImplTests END" );
    startTestMockServer( getMockServerPort() );
}

void CloudStorageImplTests::TearDownTestCase() {
    LOGGER_TRACE( "kill mock web server on pid " + cc::num10( mockServerPid() ) );
    auto forker = Process::Forker::getInstance();
    forker->kill( mockServerPid(), SIGTERM );
}

int& CloudStorageImplTests::getMockServerPort() {
    static int port = 8010;
    return port;
}

bool CloudStorageImplTests::makePingRequest() {
    LOGGER_TRACE_RAW( "ping mock server BEGIN" );
    std::string pingUrl = makeHostPortUrl( "/ping", getMockServerPort() );
    std::string pingResult;
    int connectTimeout = 1, transferTimeout = 1;
    bool pingPassed = false;

    // try to reconnect for 30 seconds, interval - 200 ms
    const std::chrono::milliseconds mockServerReconnectSleepPeriod = std::chrono::milliseconds( 200 );
    const int mockServerPingRetrySeconds = 5 * 30;

    for( int i = 0; i < mockServerPingRetrySeconds; i++ ) {
        pingResult.clear();
        pingPassed = CurlWrapper::curlCustomGet( pingUrl, {}, "", "", "", pingResult, connectTimeout, transferTimeout );
        if( !pingPassed ) {
            LOGGER_TRACE_RAW( "ping mock server, attempt  " + cc::num10( i ) + ", status: failed" );
            std::this_thread::sleep_for( mockServerReconnectSleepPeriod );
            continue;
        }
        else
            break;
    }
    LOGGER_TRACE_RAW( "ping mock server END: " + cc::flag( pingPassed ) );
    return pingPassed;
}

void CloudStorageImplTests::startTestMockServer( unsigned int port ) {
    LOGGER_INFO_RAW( "Start python mock server on port=" + cc::num10( port ) );

    // Command line:
    //  cd folder/with/server.py
    //  HOST=0.0.0.0 PORT=8010 python2 server.py --file etc/cloud_storage_config/cloud_storage_config.json

    //  catch logs and redirect them to a file

    // hard-coded one, to start with
    const std::string& firmwareSourceFolder = HubPlatform::Tests::getSourceFolder();
    const std::string mockServerPath = firmwareSourceFolder + "/firmware/common/cloud_storage/tests/mock_server/src";
    const std::string mockServerConfig = mockServerPath + "/etc/cloud_storage_config/cloud_storage_config.json";
    const auto& poligonFolder = Tests::getTestsPoligonFolder();
    std::string logFile = poligonFolder + "/simple_mock_server.log";

    auto forker = Process::Forker::getInstance();
    auto shellCommand =
        "execute() { cd " + mockServerPath + " && HOST=0.0.0.0 PORT=" + std::to_string( getMockServerPort() ) +
        " timeout 900s python2 server.py --file " + mockServerConfig + "; }; execute 1> " + logFile + " 2>&1";
    LOGGER_TRACE_RAW( "Mock server start command: " + shellCommand );
    mockServerPid() = forker->fork( shellCommand );
    LOGGER_TRACE_RAW( "Mock server created, pid: " + cc::num10( mockServerPid() ) );

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

    initializationComplete() = makePingRequest();
}

pid_t& CloudStorageImplTests::mockServerPid() {
    static pid_t pid = 0;
    return pid;
}

std::string CloudStorageImplTests::makeHostPortUrl( const std::string& path,
                                                    int port,
                                                    const std::string& host,
                                                    const std::string& protocol ) {
    return protocol + "://" + host + ":" + std::to_string( port ) + path;
}

/** Returns db record with encrypted token
 * @param port - port on which to set up mock http server
 * @param xapp - value of a xapp header to use, can be empty;
 * @param prefixes - additional path to call makeHostPortUrl with. Keys are one of:
 *      uploadUrls, uploadRedirectUrls, uploadRedirectFinishedUrls, downloadUrls, downloadRedirectUrls.
 * @sa makeHostPortUrl */
Json CloudStorageImplTests::makeCloudStorageRecordLocal( int port, const std::string& xapp, const Json& prefixes ) {
    using namespace CloudStorageInterface::Resources::DB::Fields;
    auto formatUrl = []( const Json& prefixes, const char* key, int port ) -> JsonArray {
        return JSON_ARRAY_( makeHostPortUrl( json_get_str( prefixes, key ), port ) );
    };
    Json record = { { Id, "storage_controller_token_record" },
                    { Token, "valid_non_encrypted_controller_storage_token" },
                    { TokenExpirationDate, 1673310297 },
                    { UploadRedirectFinishedUrls, formatUrl( prefixes, UploadRedirectFinishedUrls, port ) },
                    { UploadRedirectUrls, formatUrl( prefixes, UploadRedirectUrls, port ) },
                    { UploadUrls, formatUrl( prefixes, UploadUrls, port ) },
                    { UploadLogsUrls, formatUrl( prefixes, UploadLogsUrls, port ) },
                    { DownloadUrls, formatUrl( prefixes, DownloadUrls, port ) },
                    { DownloadRedirectUrls, formatUrl( prefixes, DownloadRedirectUrls, port ) },
                    { CloudApiUrls, formatUrl( prefixes, CloudApiUrls, port ) } };

    if( !xapp.empty() )
        record[ Xapp ] = xapp;

    using namespace CloudStorageInterface;
    return ControllerToken::makeDbRecord(
        xapp,
        record[ DownloadUrls ],
        record[ UploadUrls ],
        record[ UploadLogsUrls ],
        record[ DownloadRedirectUrls ],
        record[ UploadRedirectUrls ],
        record[ UploadRedirectFinishedUrls ],
        record[ CloudApiUrls ],
        record[ Token ],
        ControllerToken::ExpirationTime( json_get_int( record, TokenExpirationDate ) ) );
}

bool& CloudStorageImplTests::initializationComplete() {
    static bool initComplete = false;
    return initComplete;
}

void CloudStorageImplTests::checkInitializationComplete( int codeLine ) {
    ASSERT_TRUE( initializationComplete() )
        << "Initialization is not complete, at " << m_callingFile << ":" << codeLine;
}

void CloudStorageImplTests::callingFile( std::string file ) {
    m_callingFile = std::move( file );
}

void CloudStorageImplTests::initControllerToken( IDbClient& dbClient,
                                                 const JsonObject& decryptedTokenRecord,
                                                 int codeLine ) {
    using namespace CloudStorageInterface;
    ControllerToken controllerToken;
    ASSERT_TRUE( controllerToken.fromDbRecord( decryptedTokenRecord ) ) << __FILE__ << ":" << codeLine;
    ASSERT_TRUE( ControllerToken::insertDbRecord( dbClient, controllerToken ) ) << __FILE__ << ":" << codeLine;
}

std::string CloudStorageImplTests::makeTempFileName() {
    char tempFileName[] = "/tmp/fw_tests_cloud_storage_tmp.XXXXXX";
    close( mkstemp( tempFileName ) );
    return tempFileName;
}

TEST_F( CloudStorageImplTests, errorInvalidToken ) /* 1.1 */ {
    checkInitializationComplete( __LINE__ );
    using namespace Common::CloudStorageInterface;
    CloudStorageImpl cloudStorage( getDBClient() );
    //     Errors uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response );
    std::string filePath = "/tmp/filePath.tgz";
    std::string uuid = "0000-000000-0000-000000";
    Json metadata = { { "metadata", 1 } };

    UploadFileData fileData( filePath, uuid, metadata );
    StorageType storageType = StorageType::ArchiveControllerLog;
    CloudStorageImpl::UploadResponse uploadProcessCompleted =
        []( const Json& userData, const std::string& error, const FileKey& fileKey ) {
            LOGGER_TRACE_RAW( "UploadResponse lambda: upload completed, error: " + error +
                              ", fileKey: " + cc::j( fileKey.toJson() ) );
        };

    Errors errors = cloudStorage.uploadFile( fileData, storageType, uploadProcessCompleted );
    ASSERT_EQ( toString( errors ), toString( Errors::InvalidToken ) );
}

TEST_F( CloudStorageImplTests, errorInvalidXapp ) /* 1.2 */ {
    checkInitializationComplete( __LINE__ );

    using namespace Common::CloudStorageInterface;
    initControllerToken( getDBClient(), makeCloudStorageRecordLocal( 8081 ), __LINE__ );

    ControllerToken controllerToken;
    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    CloudStorageImpl cloudStorage( getDBClient() );
    //     Errors uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response );
    std::string filePath = "/tmp/filePath.tgz";
    std::string uuid = "0000-000000-0000-000000";
    Json metadata = { { "metadata", 1 } };

    UploadFileData fileData( filePath, uuid, metadata );
    StorageType storageType = StorageType::ArchiveControllerLog;

    CloudStorageImpl::UploadResponse uploadProcessCompleted =
        []( const Json& userData, const std::string& error, const FileKey& fileKey ) {
            LOGGER_TRACE_RAW( "UploadResponse lambda: upload completed, error: " + error +
                              ", fileKey: " + cc::j( fileKey.toJson() ) );
        };

    Errors errors = cloudStorage.uploadFile( fileData, storageType, uploadProcessCompleted );
    ASSERT_EQ( toString( errors ), toString( Errors::InvalidXapp ) );
}

bool fillFileWithData( const std::string& filePath, size_t size ) {
    std::fstream output( filePath, std::ios::binary | std::ios::out );
    if( output.is_open() ) {
        const size_t PORTION_DEFAULT = 4 * 1024;
        size_t writtenBytes = 0;
        while( writtenBytes < size ) {
            size_t portion = std::min( PORTION_DEFAULT, size - writtenBytes );
            std::string data( portion, '\0' );
            output << data;
            writtenBytes += data.size();
        }
        return true;
    }
    return false;
}

TEST_F( CloudStorageImplTests, uploadWithRedirect ) /* 2.1 */ {
    checkInitializationComplete( __LINE__ );

    using namespace Common::CloudStorageInterface;
    initControllerToken(
        getDBClient(), makeCloudStorageRecordLocal( getMockServerPort(), "non-empty-xapp" ), __LINE__ );

    ControllerToken controllerToken;
    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    CloudStorageImpl cloudStorage( getDBClient() );
    JsonObject replacements = { { "from", "http://127.0.0.1:8017" },
                                { "to", "http://127.0.0.1:" + std::to_string( getMockServerPort() ) } };
    cloudStorage.replaceExternalUrlRule( replacements );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 60 ) );
    std::string filePath = makeTempFileName();
    utils::AfterReturn onExit( [filePath]() { filesystem::removeFile( filePath ); } );

    // Fill temp file with multiple portions of data, portion = 4096
    const size_t fileSize = 4 * 1024 + 100;
    EXPECT_TRUE( fillFileWithData( filePath, fileSize ) );
    EXPECT_EQ( filesystem::getFileSize( filePath ), fileSize ) << "Created file size differs from requested file size";

    std::string uuid = "0000-000000-0000-000000";
    Json metadata = { { "metadata", 1 } };

    UploadFileData fileData( filePath, uuid, metadata );
    StorageType storageType = StorageType::ArchiveControllerLog;

    FileKey receivedFileKey = {};
    std::string status = "pending";

    std::promise< Errors > promise;
    std::future< Errors > future = promise.get_future();

    CloudStorageImpl::UploadResponse uploadProcessCompleted =
        [&status, &promise]( const Json& userData, const std::string& error, const FileKey& fileKey ) {
            LOGGER_TRACE_RAW( "UploadResponse lambda: upload completed, error: " + error +
                              ", fileKey: " + cc::j( fileKey.toJson() ) );
            if( error.empty() ) {
                status = "completed";
                promise.set_value( Errors::Ok );
            }
            else {
                status = "failed";
                LOGGER_TRACE_RAW( "uploadProcessCompleted failed, userData: " + cc::j( userData ) );
                promise.set_value( Errors::RequestError );
            }
        };

    Errors errors = cloudStorage.uploadFile( fileData, storageType, uploadProcessCompleted );
    ASSERT_EQ( toString( errors ), toString( Errors::Ok ) );

    const std::future_status futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 0 ) );

    ASSERT_EQ( status, "completed" );
    ASSERT_EQ( toString( future.get() ), toString( Errors::Ok ) );
    cloudStorage.waitTillFinish();
}

TEST_F( CloudStorageImplTests, downloadWithRedirectInOneStep ) /* 3.1 */ {
    checkInitializationComplete( __LINE__ );

    using namespace Common::CloudStorageInterface;
    auto cloudStorageRecord = makeCloudStorageRecordLocal( getMockServerPort(), "non-empty-xapp" );
    initControllerToken( getDBClient(), cloudStorageRecord, __LINE__ );

    ControllerToken controllerToken;
    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    CloudStorageImpl cloudStorage( getDBClient() );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 60 ) );

    Json input = { { "uuid", "uploaded_file_uuid" }, { "redirect_support", 1 } };

    // Make a unique autogenerated file name here
    FileKey fileKey( "", json_get_str( input, "uuid" ), "" );
    std::string downloadPath =
        std::string{ "/tmp/downloadTo." }.append( test_info_->name() ) + "." + std::to_string( getpid() );

    std::promise< Errors > promise;
    std::future< Errors > future = promise.get_future();

    std::string status = "pending";
    CloudStorageInterface::CloudStorageImpl::DownloadResponse responseCallback =
        [&status, &promise]( const Json& userData, const std::string& errorMessage ) {
            LOGGER_TRACE_RAW( "Download with redirect response callback: " + errorMessage +
                              ", userData: " + cc::j( userData ) );

            if( errorMessage.empty() )
                status = "completed";
            else
                status = "failed: " + errorMessage;

            promise.set_value( Errors::Ok );
        };

    Errors errors = cloudStorage.downloadFile( fileKey, downloadPath, responseCallback );

    ASSERT_EQ( toString( errors ), toString( Errors::Ok ) );

    const std::future_status futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 0 ) );

    ASSERT_EQ( status, "completed" );
    ASSERT_EQ( toString( future.get() ), toString( Errors::Ok ) );
    cloudStorage.waitTillFinish();
}

TEST_F( CloudStorageImplTests, downloadWithRedirect404Error ) /* 3.2 */ {
    checkInitializationComplete( __LINE__ );

    using namespace Common::CloudStorageInterface;
    using namespace Common::CloudStorageInterface::Resources::DB::Fields;
    JsonObject prefixes = { { DownloadUrls, "/download-wo-redirect" },
                            { DownloadRedirectUrls, "/download-w-redirect-404" } };

    auto cloudStorageRecord = makeCloudStorageRecordLocal( getMockServerPort(), "non-empty-xapp", prefixes );
    initControllerToken( getDBClient(), cloudStorageRecord, __LINE__ );

    ControllerToken controllerToken;
    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    CloudStorageImpl cloudStorage( getDBClient() );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 60 ) );

    Json input = { { "uuid", "uploaded_file_uuid" }, { "redirect_support", 1 } };

    // Make a unique autogenerated file name here
    FileKey fileKey( "", json_get_str( input, "uuid" ), "" );
    std::string downloadPath =
        std::string{ "/tmp/downloadTo." }.append( test_info_->name() ) + "." + std::to_string( getpid() );

    std::promise< Errors > promise;
    std::future< Errors > future = promise.get_future();

    std::string status = "pending";
    std::string message;
    CloudStorageInterface::CloudStorageImpl::DownloadResponse responseCallback =
        [&status, &message, &promise]( const Json& userData, const std::string& errorMessage ) {
            LOGGER_TRACE_RAW( "Download with redirect response callback: " + errorMessage +
                              ", userData: " + cc::j( userData ) );

            if( errorMessage.empty() ) {
                status = "completed";
                promise.set_value( Errors::Ok );
            }
            else {
                status = "failed";
                message = errorMessage;
                promise.set_value( Errors::RequestError );
            }
        };

    Errors errors = cloudStorage.downloadFile( fileKey, downloadPath, responseCallback );

    ASSERT_EQ( toString( errors ), toString( Errors::Ok ) );

    const std::future_status futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 0 ) );

    ASSERT_EQ( status, "completed" );
    ASSERT_EQ( toString( future.get() ), toString( Errors::Ok ) );
    cloudStorage.waitTillFinish();
}

TEST_F( CloudStorageImplTests, uploadWithoutRedirect ) /* 4 */ {
    checkInitializationComplete( __LINE__ );

    using namespace Common::CloudStorageInterface;
    Json cloudStorageRecord = makeCloudStorageRecordLocal( getMockServerPort(),
                                                           "non-empty-xapp",
                                                           JSON( "uploadUrls"
                                                                 << "/upload_without_redirect" ) );

    std::vector< std::string > fieldsToReset = {
        "uploadRedirectFinishedUrls", "uploadRedirectUrls", "uploadLogsUrls", "downloadRedirectUrls"
    };
    for( auto& field : fieldsToReset ) {
        cloudStorageRecord[ field ] = JSON_EMPTY_ARR;
    }

    initControllerToken( getDBClient(), cloudStorageRecord, __LINE__ );

    ControllerToken controllerToken;
    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    CloudStorageImpl cloudStorage( getDBClient() );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 60 ) );
    std::string filePath = "/tmp/filePath.tgz";
    std::string uuid = "0000-000000-0000-000000";
    Json metadata = { { "metadata", 1 } };

    UploadFileData fileData( filePath, uuid, metadata );
    StorageType storageType = StorageType::ArchiveControllerLog;

    std::string status = "pending";

    std::promise< std::pair< Errors, FileKey > > promise;
    std::future< std::pair< Errors, FileKey > > future = promise.get_future();

    CloudStorageImpl::UploadResponse uploadProcessCompleted =
        [&status, &promise]( const Json& userData, const std::string& error, const FileKey& fileKey ) {
            LOGGER_TRACE_RAW( "UploadResponse lambda: upload completed, error: " + error +
                              ", fileKey: " + cc::j( fileKey.toJson() ) );
            if( error.empty() ) {
                status = "completed";
                promise.set_value( std::make_pair( Errors::Ok, fileKey ) );
            }
            else {
                status = "failed";
                LOGGER_ERROR_RAW( "uploadProcessCompleted failed, userData: " + cc::j( userData ) );
                promise.set_value( std::make_pair( Errors::RequestError, FileKey() ) );
            }
        };

    Errors errors = cloudStorage.uploadFile( fileData, storageType, uploadProcessCompleted );
    ASSERT_EQ( toString( errors ), toString( Errors::Ok ) );

    const std::future_status futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    cloudStorage.exitDelayInterval( std::chrono::seconds( 0 ) );

    ASSERT_EQ( status, "completed" );
    const std::pair< Errors, FileKey >& uploaded = future.get();
    ASSERT_EQ( toString( uploaded.first ), toString( Errors::Ok ) );
    ASSERT_EQ( uploaded.second.uuid, "file_upload_uuid" );
    ASSERT_EQ( uploaded.second.key, "file_upload_key" );
    cloudStorage.waitTillFinish();
}

TEST_F( CloudStorageImplTests, differentBuckets ) /* 5 */ {
    auto bucket = getBucket( CloudStorageInterface::StorageType::ArchiveControllerLog );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::ControllerLogs );
    bucket = getBucket( CloudStorageInterface::StorageType::ControllerLog );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::ControllerLogs );

    bucket = getBucket( CloudStorageInterface::StorageType::ControllerBackup );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::ControllerBackups );
    bucket = getBucket( CloudStorageInterface::StorageType::Backup );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::ControllerBackups );
    bucket = getBucket( CloudStorageInterface::StorageType::ArchiveControllerBackup );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::ControllerBackups );

    bucket = getBucket( CloudStorageInterface::StorageType::Multimedia );
    ASSERT_EQ( bucket, CloudStorageInterface::StorageBucket::Media );
}

}  // namespace UT
}  // namespace Common
}  // namespace HubPlatform