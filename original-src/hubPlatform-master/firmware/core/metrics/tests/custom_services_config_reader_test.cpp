#include "test_utils/test_environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>
#include <map>

/*

TESTPLAN



*/

namespace HubPlatform {
namespace UT {

namespace Resources {
const char MetricsConfigPath[] = "/system/common/opt/firmware/etc/metrics.cfg";
}

class MetricsConfigFileManagerSuite : public ::testing::Test {
public:
    static std::string makeMetricsConfigPath();
    void fillMetricsConfigFile( const std::string& networkConfigPath, int codeLine );
    static std::string makeError( int codeLine );

protected:
    Json m_config;
    //    using ServiceFeature = std::pair< std::string, std::string >;
    /** Add a ny new service added to metrics.cfg to this collection */
    const std::map< std::string, std::string > m_serviceFeatures = {
        { "ha-mqttwebrtcd", "mqttwebrtc.v2" },
        { "ha-webrtcd", "webrtc" },
        { "mediamtx", "mediamtx.version" },
        { "ha-oracode", "" },
        { "mosquitto", "mqtt_gateway" },
        { "spread", "" },
        { "system_monitor", "system_monitor.system_monitor" },
        { "hub-log-sender", "hub-log-sender.ha-hub-log-sender" },
    };
};

std::string MetricsConfigFileManagerSuite::makeMetricsConfigPath() {
    return HubPlatform::Tests::getSourceFolder() + Resources::MetricsConfigPath;
}

void MetricsConfigFileManagerSuite::fillMetricsConfigFile( const std::string& metricsConfigPath, int codeLine ) {
    ASSERT_TRUE( filesystem::isFileExist( metricsConfigPath ) )
        << "Metrics config should exist, " << makeError( codeLine );
    auto configData = System::readFile( metricsConfigPath );
    ASSERT_FALSE( configData.empty() ) << "metrics config shouldn't be empty, " << makeError( codeLine );

    m_config = parseJson( configData );
    ASSERT_FALSE( m_config.empty() ) << "metrics config should be a parseable json file,  << makeError( codeLine )";
}

std::string MetricsConfigFileManagerSuite::makeError( int codeLine ) {
    return std::string{ "at: " }.append( __FILE__ ).append( ":" ).append( std::to_string( codeLine ) );
}

TEST_F( MetricsConfigFileManagerSuite, CheckMetricsConfigCustomServices ) /* 1.1 */ {
    auto metricsConfigPath = makeMetricsConfigPath();
    fillMetricsConfigFile( metricsConfigPath, __LINE__ );

    Json customServices, reporter;
    ASSERT_TRUE( json_get( m_config, "reporter", reporter ) ) << "reporter path should be present";
    ASSERT_TRUE( json_get( reporter, "custom_services", customServices ) ) << "custom_services path should be present";
    EXPECT_TRUE( customServices.is_array() );

    for( int i = 0; i < customServices.size(); ++i ) {
        Json service = customServices[ i ];
        std::string serviceName = json_get_str( service, "service" );

        auto foundIt = m_serviceFeatures.find( serviceName );
        EXPECT_TRUE( foundIt != m_serviceFeatures.end() ) << "Service name not found: " << serviceName;
        if( foundIt == m_serviceFeatures.end() )
            continue;

        EXPECT_EQ( serviceName, foundIt->first ) << "Service name " << serviceName;

        std::string featureName = json_get_str( service, "feature" );
        EXPECT_EQ( featureName, foundIt->second ) << "Feature name " << featureName;
    }
}

}  // namespace UT
}  // namespace HubPlatform
