#include "uid/mdns_config/mdns_config.h"
#include "uid/cluster_manager/avahi_utils.h"
#include "test_utils/temporary_root_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

namespace HubPlatform {
namespace UT {

class MdnsConfigSuite : public testing::Test {
public:
    Tests::TemporaryFolderWrapper m_folderWrapper;
    void setAvahiService( int localServerPort, const std::string& avahiServicesPath ) {
        m_mdnsConfig.setAvahiService( localServerPort, avahiServicesPath );
    }

private:
    MdnsConfig m_mdnsConfig;
};

TEST_F( MdnsConfigSuite, setMdnsConfig ) {
    const std::string servicePath = m_folderWrapper.getPath();
    setAvahiService( 17000, servicePath );
    const std::string hubServicePath = servicePath + "/hub.service";
    std::ifstream file( hubServicePath );
    EXPECT_TRUE( file.good() );
}

TEST_F( MdnsConfigSuite, parseAvahiOutputTest ) {
    std::string output =
        "+;eth0;IPv4;eZLO\\032h2\\.1\\032controller\\032\\04070010234\\041;_ezlo._tcp;local\n"
        "+;eth0;IPv4;eZLO\\032h2\\.1\\032controller\\032\\04070010195\\041;_ezlo._tcp;local\n"
        "=;eth0;IPv4;eZLO\\032h2\\.1\\032controller\\032\\04070010195\\041;_ezlo._tcp;local;HUB70010195.local;192.168."
        "42.45;17000;\"Hub Type=h2.1\" \"Vendor=eZLO\" \"Firmware Version=1.0.0\" \"Serial=70010195\"\n"
        "=;eth0;IPv4;eZLO\\032h2\\.1\\032controller\\032\\04070010234\\041;_ezlo._tcp;local;HUB70010234.local;192.168."
        "42.249;17000;\"Hub Type=h2.1\" \"Vendor=eZLO\" \"Firmware Version=1.0.0\" \"Serial=70010234\"\n";

    std::vector< Json > infos = Uid::AvahiUtils::parseAvahiBrowseOutput( output );

    ASSERT_EQ( infos.size(), 2 );
    EXPECT_EQ( "{\n"
               "    \"address\": \"192.168.42.45\",\n"
               "    \"domain\": \"local\",\n"
               "    \"hostname\": \"HUB70010195.local\",\n"
               "    \"interface\": \"eth0\",\n"
               "    \"ipVersion\": \"IPv4\",\n"
               "    \"port\": \"17000\",\n"
               "    \"records\": {\n"
               "        \"Firmware Version\": \"1.0.0\",\n"
               "        \"Hub Type\": \"h2.1\",\n"
               "        \"Serial\": \"70010195\",\n"
               "        \"Vendor\": \"eZLO\"\n"
               "    },\n"
               "    \"serviceName\": \"eZLO\\\\032h2\\\\.1\\\\032controller\\\\032\\\\04070010195\\\\041\",\n"
               "    \"serviceType\": \"_ezlo._tcp\"\n"
               "}",
               infos[ 0 ].dump( 4 ) );
    EXPECT_EQ( "{\n"
               "    \"address\": \"192.168.42.249\",\n"
               "    \"domain\": \"local\",\n"
               "    \"hostname\": \"HUB70010234.local\",\n"
               "    \"interface\": \"eth0\",\n"
               "    \"ipVersion\": \"IPv4\",\n"
               "    \"port\": \"17000\",\n"
               "    \"records\": {\n"
               "        \"Firmware Version\": \"1.0.0\",\n"
               "        \"Hub Type\": \"h2.1\",\n"
               "        \"Serial\": \"70010234\",\n"
               "        \"Vendor\": \"eZLO\"\n"
               "    },\n"
               "    \"serviceName\": \"eZLO\\\\032h2\\\\.1\\\\032controller\\\\032\\\\04070010234\\\\041\",\n"
               "    \"serviceType\": \"_ezlo._tcp\"\n"
               "}",
               infos[ 1 ].dump( 4 ) );
}

}  // namespace UT
}  // namespace HubPlatform
