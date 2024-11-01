#include "cored/model/cored_item.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Encrypt user code
DONE    2. Decrypt user code
DONE    3. Encrypt user codes dictionary
DONE    4. Decrypt user codes dictionary

*/

namespace HubPlatform {
namespace CoreD {
namespace UT {

using namespace ::testing;

class ItemsEncryptionTestSuite : public Test {
public:
    ItemsEncryptionTestSuite() {
        m_provisionData.setControllerKey( "\x38\xD3\xEB\x18\x59\x8D\x44\x4E\xEE\x41\x0B\x3E\x70\xC9\xFA\xB7\xA2\xF8\x3C"
                                          "\x90\x4E\x2C\x16\xFA\xC1\xF1\xE5\xD4\x86\xEF\x8B" );
    }

    const ProvisionDataUtils::ProvisionData& getProvisionData() const {
        return m_provisionData;
    }

private:
    ProvisionDataUtils::ProvisionData m_provisionData;
};

TEST_F( ItemsEncryptionTestSuite, encryptUserCode /*1*/ ) {
    auto itemJson = Json::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac04",
        "type": "userCode",
        "value": {
            "code": "1984",
            "name": "1"
        }
    })" );

    Model::Item::encryptItemAsJson( getProvisionData(), itemJson );

    EXPECT_EQ( itemJson[ "value" ][ "code" ].get< std::string >().size(), 44 );
}

TEST_F( ItemsEncryptionTestSuite, decryptUserCode /*2*/ ) {
    auto itemJson = Json::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac04",
        "type": "userCode",
        "value": {
            "code": "4QDyOvIoGdwp0tgo0jK0VfCj8xMvVrzfLhAsHIjoZo0=",
            "name": "1"
        }
    })" );

    Model::Item::decryptItemAsJson( getProvisionData(), itemJson );

    EXPECT_EQ( itemJson[ "value" ][ "code" ].get< std::string >(), "1984" );
}

TEST_F( ItemsEncryptionTestSuite, encryptUserCodesDictionary /*3*/ ) {
    auto itemJson = Json::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac04",
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "9999",
                "name": "1"
            },
            "44": {
                "code": "4444",
                "name": "44"
            }
        }
    })" );

    Model::Item::encryptItemAsJson( getProvisionData(), itemJson );

    EXPECT_EQ( itemJson[ "value" ][ "1" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( itemJson[ "value" ][ "44" ][ "code" ].get< std::string >().size(), 44 );
}

TEST_F( ItemsEncryptionTestSuite, decryptUserCodesDictionary /*4*/ ) {
    auto itemJson = Json::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac04",
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "j7NV2lXIhTGj+ZB8Yu0R/ilRqeFzu5bXOO2M/tk/HV0=",
                "name": "1"
            },
            "44": {
                "code": "/PpX7APSHRyg5mUsB8v/AVgh9Qfl8XPOi+Sm5bueSrE=",
                "name": "44"
            }
        }
    })" );

    Model::Item::decryptItemAsJson( getProvisionData(), itemJson );

    EXPECT_EQ( itemJson[ "value" ][ "1" ][ "code" ].get< std::string >(), "9999" );
    EXPECT_EQ( itemJson[ "value" ][ "44" ][ "code" ].get< std::string >(), "4444" );
}

}  // namespace UT
}  // namespace CoreD
}  // namespace HubPlatform
