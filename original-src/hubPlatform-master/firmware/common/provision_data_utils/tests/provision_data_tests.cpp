#include "provision_data_utils/provision_data.h"

#include "json_utils/json_wrapper.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Invalid data
DONE    2. Valid data
DONE    3. From json
DONE        3.1 Missing fields
DONE        3.2 Format rev.1
DONE        3.3 Format rev.2
DONE    4. To json
DONE        3.1 Missing fields
DONE        3.2 Correct rev.2

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::ProvisionDataUtils;

class ProvisionDataTestSuite : public ::testing::Test {};

TEST_F( ProvisionDataTestSuite, InvalidData /*1*/ ) {
    ProvisionData data;
    EXPECT_FALSE( data.isValid() );
}

TEST_F( ProvisionDataTestSuite, ValidData /*2*/ ) {
    ProvisionData data;
    data.configVersion = 1;
    EXPECT_TRUE( data.isValid() );
}

TEST_F( ProvisionDataTestSuite, FromJsonMissingFields /*3.1*/ ) {
    auto json = JSON( "config_version" << 1 << "provision_server"
                                       << "https://req-disp-g150.mios.com/" );
    ProvisionData data;
    data.fillFromJson( json );
    EXPECT_TRUE( data.isValid() );
    EXPECT_EQ( data.configVersion, 1 );
    EXPECT_EQ( data.provisionServer, "https://req-disp-g150.mios.com/" );
    EXPECT_TRUE( data.cloudServer.empty() );
}

TEST_F( ProvisionDataTestSuite, FromJsonFormatRev1 /*3.2*/ ) {
    auto json = JSON( "ca_certificate"
                      << "-----BEGIN CERTIFICATE-----\\nMIICb...\\n-----END CERTIFICATE-----"
                      << "cloud_server"
                      << "https://cloud.ezlo.com:7000"
                      << "default_ssh_pass"
                      << "$1$ioHJM0hg$fVcIDx0aAFGedU7aiGp0b0"
                      << "default_wifi_pass"
                      << "a5f595f63e46e45fc441ce40"
                      << "default_wifi_ssid"
                      << "ezlo_smth"
                      << "id"
                      << "70010234"
                      << "hardware_revision"
                      << "hw_rev_1.0"
                      << "provision_server"
                      << "https://req-disp-at0m.mios.com/"
                      << "provisioning_token"
                      << "f367d..."
                      << "ssl_certificate"
                      << "\\n-----BEGIN CERTIFICATE-----\\nMIICC...\\n-----END CERTIFICATE-----\\n"
                      << "ssl_private_key"
                      << "-----BEGIN PRIVATE KEY-----\\nMIGEA...\\n-----END PRIVATE KEY-----\\n"
                      << "updates_repository"
                      << ""
                      << "uuid"
                      << "6c276270-86af-11e9-832b-6d4e42a8c158"
                      << "ver" << 1 << "zwave_region"
                      << "US" );
    ProvisionData data;
    data.fillFromJson( json );
    EXPECT_TRUE( data.isValid() );

    EXPECT_EQ( data.id, 70010234 );
    EXPECT_EQ( data.uuid, "6c276270-86af-11e9-832b-6d4e42a8c158" );
    EXPECT_EQ( data.hardwareRevision, "hw_rev_1.0" );
    EXPECT_EQ( data.configVersion, 1 );
    EXPECT_EQ( data.zwaveRegion, "US" );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud.ezlo.com:7000" );
    EXPECT_EQ( data.provisionToken, "f367d..." );
    EXPECT_EQ( data.sslPrivateKey, "-----BEGIN PRIVATE KEY-----\nMIGEA...\n-----END PRIVATE KEY-----\n" );
    EXPECT_EQ( data.sslSharedKey, "\n-----BEGIN CERTIFICATE-----\nMIICC...\n-----END CERTIFICATE-----\n" );
    EXPECT_EQ( data.signingCaCertificate, "-----BEGIN CERTIFICATE-----\nMIICb...\n-----END CERTIFICATE-----" );
    EXPECT_EQ( data.defaultSshPassword, "$1$ioHJM0hg$fVcIDx0aAFGedU7aiGp0b0" );
    EXPECT_EQ( data.defaultWifiSsid, "ezlo_smth" );
    EXPECT_EQ( data.defaultWifiPassword, "a5f595f63e46e45fc441ce40" );
}

TEST_F( ProvisionDataTestSuite, FromJsonFormatRev2 /*3.3*/ ) {
    auto json = JSON( "id" << 70010234 << "uuid"
                           << "6c276270-86af-11e9-832b-6d4e42a8c158"
                           << "hardware_revision"
                           << "hw_rev_1.0"
                           << "cloud_uuid"
                           << "7bf52bf1-dcab-4ada-afcd-8483bc410a9d"
                           << "order_uuid"
                           << "670e4ce0-86af-11e9-992b-d3f657efc9b2"
                           << "config_version" << 1 << "zwave_region" << JSON_ARRAY( "US" ) << "provision_server"
                           << "https://req-disp-at0m.mios.com/"
                           << "cloud_server"
                           << "https://cloud.ezlo.com:7000"
                           << "provision_token"
                           << "f367d..."
                           << "provision_order" << 1652383 << "ssl_private_key"
                           << "-----BEGIN PRIVATE KEY-----\\nMIGEA...\\n-----END PRIVATE KEY-----\\n"
                           << "ssl_public_key"
                           << "-----BEGIN PUBLIC KEY-----\\nMFYwE...\\n-----END PUBLIC KEY-----"
                           << "ssl_shared_key"
                           << "\\n-----BEGIN CERTIFICATE-----\\nMIICC...\\n-----END CERTIFICATE-----\\n"
                           << "signing_ca_certificate"
                           << "-----BEGIN CERTIFICATE-----\\r\\nMIICb...\\r\\n-----END CERTIFICATE-----"
                           << "default_wifi_ssid"
                           << "ezlo_smth"
                           << "default_wifi_password"
                           << "a5f595f63e46e45fc441ce40" );
    ProvisionData data;
    data.fillFromJson( json );
    EXPECT_TRUE( data.isValid() );

    EXPECT_EQ( data.id, 70010234 );
    EXPECT_EQ( data.uuid, "6c276270-86af-11e9-832b-6d4e42a8c158" );
    EXPECT_EQ( data.hardwareRevision, "hw_rev_1.0" );
    EXPECT_EQ( data.cloudUuid, "7bf52bf1-dcab-4ada-afcd-8483bc410a9d" );
    EXPECT_EQ( data.orderUuid, "670e4ce0-86af-11e9-992b-d3f657efc9b2" );
    EXPECT_EQ( data.configVersion, 1 );
    EXPECT_EQ( data.zwaveRegion, "US" );
    EXPECT_EQ( data.provisionServer, "https://req-disp-at0m.mios.com/" );
    EXPECT_EQ( data.cloudServer, "https://cloud.ezlo.com:7000" );
    EXPECT_EQ( data.provisionToken, "f367d..." );
    EXPECT_EQ( data.provisionOrder, 1652383 );
    EXPECT_EQ( data.sslPrivateKey, "-----BEGIN PRIVATE KEY-----\nMIGEA...\n-----END PRIVATE KEY-----\n" );
    EXPECT_EQ( data.sslPublicKey, "-----BEGIN PUBLIC KEY-----\nMFYwE...\n-----END PUBLIC KEY-----" );
    EXPECT_EQ( data.sslSharedKey, "\n-----BEGIN CERTIFICATE-----\nMIICC...\n-----END CERTIFICATE-----\n" );
    EXPECT_EQ( data.signingCaCertificate, "-----BEGIN CERTIFICATE-----\nMIICb...\n-----END CERTIFICATE-----" );
    EXPECT_EQ( data.defaultWifiSsid, "ezlo_smth" );
    EXPECT_EQ( data.defaultWifiPassword, "a5f595f63e46e45fc441ce40" );
    EXPECT_EQ( data.defaultSshPassword, "" );
}

TEST_F( ProvisionDataTestSuite, ToJsonMissingFields /*4.1*/ ) {
    ProvisionData data;
    data.configVersion = 2;
    data.cloudServer = "https://cloud-qa.ezlo.com:7000";

    EXPECT_EQ( data.toJson().dump( 4 ),
               "{\n"
               "    \"cloud_server\": \"https://cloud-qa.ezlo.com:7000\",\n"
               "    \"config_version\": 2\n"
               "}" );
}

TEST_F( ProvisionDataTestSuite, ToJsonCorrectRev2 /*4.2*/ ) {
    ProvisionData data;
    data.configVersion = 1;
    data.id = 70010234;
    data.uuid = "6c276270-86af-11e9-832b-6d4e42a8c158";
    data.hardwareRevision = "hw_rev_1.0";
    data.cloudUuid = "7bf52bf1-dcab-4ada-afcd-8483bc410a9d";
    data.orderUuid = "670e4ce0-86af-11e9-992b-d3f657efc9b2";
    data.cloudServer = "https://cloud.ezlo.com:7000";
    data.provisionServer = "https://req-disp-at0m.mios.com/";
    data.provisionToken = "f367d...";
    data.provisionOrder = 1652383;
    data.sslPrivateKey = R"(-----BEGIN PRIVATE KEY-----\nMIGEA...\n-----END PRIVATE KEY-----\n)";
    data.sslPublicKey = R"(-----BEGIN PUBLIC KEY-----\nMFYwE...\n-----END PUBLIC KEY-----)";
    data.sslSharedKey = R"(\n-----BEGIN CERTIFICATE-----\nMIICC...\n-----END CERTIFICATE-----\n)";
    data.signingCaCertificate = R"(-----BEGIN CERTIFICATE-----\r\nMIICb...\r\n-----END CERTIFICATE-----)";
    data.zwaveRegion = "US";
    data.defaultSshPassword = "$1$ioHJM0hg$fVcIDx0aAFGedU7aiGp0b0";
    data.defaultWifiSsid = "ezlo_smth";
    data.defaultWifiPassword = "a5f595f63e46e45fc441ce40";
    data.timeServers = { "0.pool.ntp.org", "1.pool.ntp.org" };

    EXPECT_EQ(
        data.toJson().dump( 4 ),
        "{\n"
        "    \"cloud_server\": \"https://cloud.ezlo.com:7000\",\n"
        "    \"cloud_uuid\": \"7bf52bf1-dcab-4ada-afcd-8483bc410a9d\",\n"
        "    \"config_version\": 1,\n"
        "    \"default_ssh_password\": \"$1$ioHJM0hg$fVcIDx0aAFGedU7aiGp0b0\",\n"
        "    \"default_wifi_password\": \"a5f595f63e46e45fc441ce40\",\n"
        "    \"default_wifi_ssid\": \"ezlo_smth\",\n"
        "    \"hardware_revision\": \"hw_rev_1.0\",\n"
        "    \"id\": 70010234,\n"
        "    \"order_uuid\": \"670e4ce0-86af-11e9-992b-d3f657efc9b2\",\n"
        "    \"provision_order\": 1652383,\n"
        "    \"provision_server\": \"https://req-disp-at0m.mios.com/\",\n"
        "    \"provision_token\": \"f367d...\",\n"
        "    \"signing_ca_certificate\": \"-----BEGIN CERTIFICATE-----\\\\r\\\\nMIICb...\\\\r\\\\n-----END "
        "CERTIFICATE-----\",\n"
        "    \"ssl_private_key\": \"-----BEGIN PRIVATE KEY-----\\\\nMIGEA...\\\\n-----END PRIVATE KEY-----\\\\n\",\n"
        "    \"ssl_public_key\": \"-----BEGIN PUBLIC KEY-----\\\\nMFYwE...\\\\n-----END PUBLIC KEY-----\",\n"
        "    \"ssl_shared_key\": \"\\\\n-----BEGIN CERTIFICATE-----\\\\nMIICC...\\\\n-----END "
        "CERTIFICATE-----\\\\n\",\n"
        "    \"time_servers\": [\n"
        "        \"0.pool.ntp.org\",\n"
        "        \"1.pool.ntp.org\"\n"
        "    ],\n"
        "    \"uuid\": \"6c276270-86af-11e9-832b-6d4e42a8c158\",\n"
        "    \"zwave_region\": [\n"
        "        \"US\"\n"
        "    ]\n"
        "}" );
}
