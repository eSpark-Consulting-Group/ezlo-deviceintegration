#include "cloud_storage/controller_token.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class ControllerTokenTests : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
    IDbClient& getDBClient();

protected:
    std::unique_ptr< IDbClient > m_dbClient;
};

void ControllerTokenTests::SetUp() {
    m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
}

void ControllerTokenTests::TearDown() {
    m_dbClient.reset();
}

IDbClient& ControllerTokenTests::getDBClient() {
    return *m_dbClient.get();
}

TEST_F( ControllerTokenTests, initDbRecord ) {
    using namespace Common::CloudStorageInterface;
    ControllerToken token;

    ControllerToken::initDbRecord( getDBClient() );

    ControllerToken::fetchDbRecord( getDBClient(), token );

    auto expectedId = token.getDbRecordId();
    auto actualId = json_get_str( token.getRawData(), "_id" );
    EXPECT_EQ( actualId, expectedId );

    auto result = getDBClient().findOne( TempCloudStorageDBName, JSON( "_id" << token.getDbRecordId() ), nullptr );
    ASSERT_FALSE( result.empty() );
    LOGGER_INFO( cc::j( result ) );
    EXPECT_EQ( token.getExpirationTime().count(), 0 );
}

namespace Resources {
const Json storageControllerToken = R"({
    "endpoints": {
        "download": [
            "https://download-cloud-qa.ezlo.com"
        ],
        "upload": [
            "https://upload-cloud-qa.ezlo.com"
        ],
        "cloud_api": [
            "https://cloud-api-cloud-qa.ezlo.com"
        ],
        "unknown": [
            "https://unknown-cloud-qa.ezlo.com"
        ]
    },
    "token": {
        "expires_ts": 2147483647,
        "string": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
    },
    "xapp": "xapp-value-uuid"
} )"_json;
}

TEST_F( ControllerTokenTests, updateDbRecord ) {
    using namespace Common::CloudStorageInterface;
    ControllerToken::initDbRecord( getDBClient() );

    ControllerToken controllerToken;
    controllerToken.fromApiRecord( Resources::storageControllerToken );

    ControllerToken::updateDbRecord( getDBClient(), controllerToken );

    ASSERT_TRUE( ControllerToken::fetchDbRecord( getDBClient(), controllerToken ) );

    std::string selectedUrl;
    const auto& endpoints = controllerToken.getEndpoints();
    EXPECT_FALSE( endpoints.hasUploadRedirectSupport() );
    EXPECT_FALSE( endpoints.hasDownloadRedirectSupport() );
    EXPECT_GT( endpoints.selectDownloadUrl( selectedUrl ), -1 );

    Json expectedUrl;
    EXPECT_TRUE( json_get( Resources::storageControllerToken, { "endpoints", "download", 0 }, expectedUrl ) );
    EXPECT_EQ( selectedUrl, json_get_str( expectedUrl ) );

    EXPECT_TRUE( json_get( Resources::storageControllerToken, { "endpoints", "upload", 0 }, expectedUrl ) );
    EXPECT_GT( endpoints.selectUploadUrl( selectedUrl ), -1 );
    EXPECT_EQ( selectedUrl, json_get_str( expectedUrl ) );

    EXPECT_TRUE( json_get( Resources::storageControllerToken, { "endpoints", "cloud_api", 0 }, expectedUrl ) );
    EXPECT_GT( endpoints.selectCloudApiUrl( selectedUrl ), -1 );
    EXPECT_EQ( selectedUrl, json_get_str( expectedUrl ) );

    EXPECT_TRUE( json_get( Resources::storageControllerToken, { "endpoints", "unknown", 0 }, expectedUrl ) );
}

TEST_F( ControllerTokenTests, fetchTokenString ) {
    using namespace Common::CloudStorageInterface;
    ControllerToken::initDbRecord( getDBClient() );

    ControllerToken controllerToken;
    controllerToken.fromApiRecord( Resources::storageControllerToken );

    auto tokenExpirationDateSec = controllerToken.getExpirationTime();
    const int expectedExpiresTs = 2147483647;
    EXPECT_EQ( tokenExpirationDateSec.count(), std::chrono::seconds( expectedExpiresTs ).count() );
    EXPECT_EQ( controllerToken.getTokenValue(), "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" );
    EXPECT_EQ( controllerToken.getXAppValue(), "xapp-value-uuid" );
}

}  // namespace UT
}  // namespace HubPlatform
