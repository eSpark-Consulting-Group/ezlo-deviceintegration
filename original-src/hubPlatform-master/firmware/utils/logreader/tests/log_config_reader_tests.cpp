#include "config_reader/tests/config_reader_factory.h"
#include "json_utils/json_wrapper.h"
#include "logreader/log_config_reader.h"
#include "system_utils/environment.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace LogReader {
namespace UT {

using ConfigFactory = HubPlatform::UT::ConfigReaderFactory;

class LogConfigReaderTestSuite : public testing::Test, public ConfigFactory {
public:
    LogConfigReaderTestSuite() = default;

    void runTest( std::vector< char* > argv, std::string config, std::function< void( LogConfigReader&& ) > func ) {
        func( getLogConfigReader( config, argv.size() - 1, argv.data() ) );
    }

    void ubusTest( std::string serviceName,
                   std::string serverName,
                   std::string callbackName,
                   std::function< void( LogConfigReader&& ) > func ) {
        std::string config =
            JSON( "ubus" << JSON( "state" << true << "server_name" << serverName << "callback_name" << callbackName ) )
                .dump();
        std::vector< char* > argv = {
            (char*) "logreader", (char*) "--service-name", (char*) serviceName.data(), nullptr
        };
        runTest( argv, config, func );

        argv = { (char*) "logreader",
                 (char*) "--ubus-state",
                 (char*) "true",
                 (char*) "--service-name",
                 (char*) serviceName.data(),
                 (char*) "--server-name",
                 (char*) serverName.data(),
                 (char*) "--callback-name",
                 (char*) callbackName.data(),
                 nullptr };
        runTest( argv, "", func );
    }

private:
    LogConfigReader getLogConfigReader( std::string config, int argc, char* argv[] ) {
        return LogConfigReader( getConfigReader( config ), argc, argv );
    }

    ConfigReader getConfigReader( std::string config ) {
        return ConfigFactory::createConfigReader( [config]( const std::string& ) { return config; } );
    }
};

TEST_F( LogConfigReaderTestSuite, MissingServiceName ) {
    std::string config = JSON( "stdout" << JSON( "state" << true ) ).dump();
    auto func = []( LogConfigReader&& logConfigReader ) {
        EXPECT_EQ( logConfigReader.getServiceName(), "" );
        EXPECT_FALSE( logConfigReader.isValid() );
    };

    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", nullptr };
    runTest( argv, config, func );

    argv = { (char*) "logreader", (char*) "ha-service", nullptr };
    runTest( argv, config, func );
}

TEST_F( LogConfigReaderTestSuite, SuccessServiceName ) {
    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", (char*) "ha-test", nullptr };
    std::string config = JSON( "stdout" << JSON( "state" << true ) ).dump();
    runTest( argv, config, []( LogConfigReader&& logConfigReader ) { EXPECT_TRUE( logConfigReader.isValid() ); } );
}

TEST_F( LogConfigReaderTestSuite, DisableAllLogWriter ) {
    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", (char*) "ha-test", nullptr };
    runTest( argv, "", []( LogConfigReader&& logConfigReader ) { EXPECT_FALSE( logConfigReader.isValid() ); } );
}

TEST_F( LogConfigReaderTestSuite, DisableStdOutConfig ) {
    std::vector< char* > argv = { (char*) "logreader",      (char*) "--service-name", (char*) "ha-test",
                                  (char*) "--stdout-state", (char*) "false",          nullptr };
    auto func = []( LogConfigReader&& logConfigReader ) {
        EXPECT_FALSE( logConfigReader.isValid() );
        EXPECT_FALSE( logConfigReader.getStdOutLogState() );
        EXPECT_FALSE( logConfigReader.getFileLogState() );
        EXPECT_FALSE( logConfigReader.getUbusLogState() );
    };

    runTest( argv, JSON( "stdout" << JSON( "state" << false ) ).dump(), func );
    runTest( argv, JSON( "stdout" << JSON( "state" << true ) ).dump(), func );
}

TEST_F( LogConfigReaderTestSuite, EnableStdOutConfig ) {
    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", (char*) "ha-test", nullptr };
    auto func = []( LogConfigReader&& logConfigReader ) {
        EXPECT_TRUE( logConfigReader.isValid() );
        EXPECT_TRUE( logConfigReader.getStdOutLogState() );
        EXPECT_FALSE( logConfigReader.getFileLogState() );
        EXPECT_FALSE( logConfigReader.getUbusLogState() );
    };

    runTest( argv, JSON( "stdout" << JSON( "state" << true ) ).dump(), func );

    argv = { (char*) "logreader",      (char*) "--service-name", (char*) "ha-test",
             (char*) "--stdout-state", (char*) "true",           nullptr };
    runTest( argv, JSON( "stdout" << JSON( "state" << false ) ).dump(), func );
}

TEST_F( LogConfigReaderTestSuite, EnableFileConfig ) {
    std::string serviceName = "ha-test";
    std::string logFilePath = Environment::getLogsFolder() + "/" + serviceName + ".log";
    std::string config = JSON( "file" << JSON( "state" << true << "log_file_path"
                                                       << "" ) )
                             .dump();

    auto func = [&]( LogConfigReader&& logConfigReader ) {
        EXPECT_TRUE( logConfigReader.isValid() );
        EXPECT_TRUE( logConfigReader.getFileLogState() );
        EXPECT_EQ( logConfigReader.getLogFilePath(), logFilePath );
        EXPECT_FALSE( logConfigReader.getStdOutLogState() );
        EXPECT_FALSE( logConfigReader.getUbusLogState() );
    };

    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", (char*) serviceName.data(), nullptr };
    runTest( argv, config, func );

    argv = { (char*) "logreader",    (char*) "--service-name", (char*) "ha-test",
             (char*) "--file-state", (char*) "true",           nullptr };
    runTest( argv, JSON( "stdout" << JSON( "state" << false ) ).dump(), func );

    logFilePath = "/test_log_dir/test_log_file";
    argv = { (char*) "logreader", (char*) "--service-name",  (char*) "ha-test",          (char*) "--file-state",
             (char*) "true",      (char*) "--log-file-path", (char*) logFilePath.data(), nullptr };
    runTest( argv, JSON( "stdout" << JSON( "state" << false ) ).dump(), func );
}

TEST_F( LogConfigReaderTestSuite, IncorrectUbusConfig ) {
    std::string serviceName = "ha-test";
    std::string serverName = "";
    std::string callbackName = "";

    auto func = [&]( LogConfigReader&& logConfigReader ) {
        EXPECT_FALSE( logConfigReader.isValid() );
        EXPECT_TRUE( logConfigReader.getUbusLogState() );
        EXPECT_EQ( logConfigReader.getUbusServerName(), serverName );
        EXPECT_EQ( logConfigReader.getUbusCallbackName(), callbackName );
        EXPECT_FALSE( logConfigReader.getStdOutLogState() );
        EXPECT_FALSE( logConfigReader.getFileLogState() );
    };

    std::string config = JSON( "ubus" << JSON( "state" << true ) ).dump();
    std::vector< char* > argv = { (char*) "logreader", (char*) "--service-name", (char*) serviceName.data(), nullptr };
    runTest( argv, config, func );

    std::vector< std::pair< std::string, std::string > > args = {
        { "", "" }, { "test_server_name", "" }, { "", "test_callback_name" }
    };

    for( const auto& it : args ) {
        serverName = it.first;
        callbackName = it.second;
        ubusTest( serviceName, serverName, callbackName, func );
    }
}

TEST_F( LogConfigReaderTestSuite, EnableUbusConfig ) {
    std::string serviceName = "ha-test";
    std::string serverName = "test_server_name";
    std::string callbackName = "test_callback_name";

    auto func = [&]( LogConfigReader&& logConfigReader ) {
        EXPECT_TRUE( logConfigReader.isValid() );
        EXPECT_TRUE( logConfigReader.getUbusLogState() );
        EXPECT_EQ( logConfigReader.getUbusServerName(), serverName );
        EXPECT_EQ( logConfigReader.getUbusCallbackName(), callbackName );
        EXPECT_FALSE( logConfigReader.getStdOutLogState() );
        EXPECT_FALSE( logConfigReader.getFileLogState() );
    };

    ubusTest( serviceName, serverName, callbackName, func );
}

}  // namespace UT
}  // namespace LogReader
}  // namespace HubPlatform
