#include "watcher/resources/watcher_resources.h"

#include "json_utils/json_fwd.h"
#include "json_utils/json_utils.h"
#include "system_utils/system.h"
#include "config_reader/config_reader.h"
#include "config_reader/tests/config_reader_factory.h"
#include "test_utils/test_environment.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

namespace Resources {
const char* watcherConfigPath = "/system/common/opt/firmware/etc/watcher.cfg";
}

class WatcherConfigTestSuite : public ::testing::Test {
public:
    std::string getWatcherConfigPath() {
        return Tests::getSourceFolder() + Resources::watcherConfigPath;
    }

    std::string readConfigFile() {
        return System::readFile( getWatcherConfigPath() );
    }

    class ConfigReaderFactoryImpl : public ConfigReaderFactory {
    public:
        using ConfigReaderFactory::createConfigReader;
    };

    ConfigReader createConfigReader() {
        return ConfigReaderFactoryImpl::createConfigReader( [this]( const std::string& ) { return readConfigFile(); } );
    }
};

TEST_F( WatcherConfigTestSuite, checkWatcherConfigFileParsible ) {
    JsonObject watcherConfigJson;

    EXPECT_TRUE( parseJson( readConfigFile(), watcherConfigJson ) );
}

TEST_F( WatcherConfigTestSuite, checkWatcherConfigUninterruptibleSleepHandler ) {
    const auto timeoutKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersUninterruptibleSleepTimeout;
    const auto checkIntervalKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersUninterruptibleSleepCheckInterval;

    ConfigReader config = createConfigReader();

    int intValue = 0;

    EXPECT_NO_THROW( intValue = config.getInt( timeoutKey ) );
    EXPECT_GT( intValue, 0 );

    EXPECT_NO_THROW( intValue = config.getInt( checkIntervalKey ) );
    EXPECT_GT( intValue, 0 );
}

TEST_F( WatcherConfigTestSuite, checkWatcherConfigZWaveNodeExpiredHandler ) {
    const auto timeoutKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersZWaveNodeExpiredTimeout;
    const auto checkIntervalKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersZWaveNodeExpiredCheckInterval;

    ConfigReader config = createConfigReader();

    int intValue = 0;

    EXPECT_NO_THROW( intValue = config.getInt( timeoutKey ) );
    EXPECT_GT( intValue, 0 );

    EXPECT_NO_THROW( intValue = config.getInt( checkIntervalKey ) );
    EXPECT_GT( intValue, 0 );
}

TEST_F( WatcherConfigTestSuite, checkWatcherConfigUnresponsiveServicesHandler ) {
    const auto timeoutKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesTimeout;
    const auto serviceTimeoutKey =
        Watcher::Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesServiceTimeout;
    const auto checkIntervalKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersUnresponsiveServicesCheckInterval;

    ConfigReader config = createConfigReader();

    int intValue = 0;

    EXPECT_NO_THROW( intValue = config.getInt( timeoutKey ) );
    EXPECT_GE( intValue, 0 );

    EXPECT_NO_THROW( intValue = config.getInt( checkIntervalKey ) );
    EXPECT_GT( intValue, 0 );

    // Map of string:int pairs
    Json serviceTimeoutJson;
    EXPECT_NO_THROW( serviceTimeoutJson = config.getJsonObject( ConfigReader::Path( config, serviceTimeoutKey ) ) );
    ASSERT_TRUE( serviceTimeoutJson.is_object() );

    for( auto it = serviceTimeoutJson.begin(); it != serviceTimeoutJson.end(); ++it ) {
        EXPECT_FALSE( it.key().empty() ) << "Empty name is not allowd";
        ASSERT_TRUE( it.value().is_number() ) << "Value is not integer: key: " + it.key();
        EXPECT_GE( int( it.value() ), 0 );
    }
}

TEST_F( WatcherConfigTestSuite, checkWatcherConfigWifiBcdcErrorHandler ) {
    const auto checkIntervalKey = Watcher::Resources::ConfigKeys::WatcherIssueHandlersWifiBcdcErrorCheckInterval;

    ConfigReader config = createConfigReader();

    int intValue = 0;
    EXPECT_NO_THROW( intValue = config.getInt( checkIntervalKey ) );
    EXPECT_GT( intValue, 0 );
}

}  // namespace UT
}  // namespace HubPlatform
