#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Hash tests
DONE        1.1 md5
DONE            1.1.1 md5, text input
DONE            1.1.2 md5, binary input
DONE        1.2 sha1
DONE        1.3 to base64
DONE        1.4 from base64
DONE            1.4.1 from base64, binary result
DONE        1.5 sha256
DONE        1.6 digest_by_name
DONE            1.6.1 sha1 by name
DONE            1.6.2 sha224 by name
DONE            1.6.3 sha256 by name
DONE            1.6.4 sha384 by name
DONE            1.6.5 sha512 by name
DONE            1.6.6 md4 by name
DONE            1.6.7 dss1 by name
DONE            1.6.8 ripemd160 by name
DONE        1.7 get_all_digest_names
DONE        1.8 from_hex_string
DONE            1.8.1 from_hex_string ok
DONE            1.8.2 from_hex_string fails

DONE    2. AES cipher
DONE        2.1 encryption
DONE            2.1.1 different cipher types
DONE            2.1.2 binary data
DONE            2.1.3 lua data
DONE        2.2 decryption
DONE            2.2.1 different cipher types
DONE            2.3.2 binary data
DONE        2.3 creating failure
DONE            2.3.1 wrong key size
DONE            2.3.2 unknown mode
DONE            2.3.3 bad cipher
DONE            2.3.4 no data

*/

namespace HubPlatform {
namespace Tests {

class LuadSecurityModuleTests : public CoreNmaTest {
public:
    void SetUp() override;
    void hashRequest( const std::string& method,
                      const std::string& digestName,
                      const std::string& message,
                      int codeLine );
};

void LuadSecurityModuleTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

namespace Resources {
static char securityTestScript[] = "HUB:security_test/scripts/security_test";
static char encryptScript[] = "HUB:security_test/scripts/encrypt";
static char encryptLuaDataScript[] = "HUB:security_test/scripts/encrypt_lua_data";
static char decryptScript[] = "HUB:security_test/scripts/decrypt";
static char toBase64[] = "HUB:security_test/scripts/to_base64";
static char fromBase64[] = "HUB:security_test/scripts/from_base64";

const char* const TestMessage = "This is a string";
}  // namespace Resources

using namespace Resources;

void LuadSecurityModuleTests::hashRequest( const std::string& method,
                                           const std::string& digestName,
                                           const std::string& message,
                                           int codeLine ) {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", Resources::securityTestScript } } );

    request[ "params" ][ "scriptParams" ] = JSON_EMPTY_OBJ;

    request[ "params" ][ "scriptParams" ][ "hash" ] = digestName;
    request[ "params" ][ "scriptParams" ][ "data" ] = message;
    if( !method.empty() )
        request[ "params" ][ "scriptParams" ][ "method" ] = method;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 ) << getCodeLineString( codeLine );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_md5 ) {  // 1.1.1
    auto expectedValue = "41fb5b5ae4d57c5ee528adb00e5e8e74";
    auto inputData = TestMessage;
    hashRequest( "", "md5", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_md5_binary ) {  // 1.1.2
    auto inputData = "&";                                     // will be transformed on a Lua script to : "\0&"
    auto expectedValue = "2167d59269eec304f5792d96a7e536e4";

    hashRequest( "", "md5_binary", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_sha1 ) {  // 1.2
    auto inputData = TestMessage;
    auto expectedValue = "f72017485fbf6423499baf9b240daa14f5f095a1";

    hashRequest( "", "sha1", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_ToBase64 ) {  // 1.3
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", toBase64 } } );

    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "data" ] = "hello";
    checkResult( request, "aGVsbG8=", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_FromBase64 ) {  // 1.4
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fromBase64 } } );

    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "data" ] = "aGVsbG8=";
    checkResult( request, "hello", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_FromBase64_binary ) {  // 1.4.1
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", fromBase64 } } );

    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "data" ] = "ACY=";  // == to_base64('\0&')
    scriptParams[ "method" ] = "md5";

    auto expectMD5Hash = "2167d59269eec304f5792d96a7e536e4";
    checkResult( request, expectMD5Hash, __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_sha256 ) {  // 1.5
    auto inputData = TestMessage;
    auto expectedValue = "4e9518575422c9087396887ce20477ab5f550a4aa3d161c5c22a996b0abb8b35";
    hashRequest( "", "sha256", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_sha1 ) {  // 1.6.1
    const auto inputData = TestMessage;
    const auto expectedValue = "f72017485fbf6423499baf9b240daa14f5f095a1";
    hashRequest( "digest_by_name", "sha1", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_sha224 ) {  // 1.6.2
    const auto inputData = TestMessage;
    const auto expectedValue = "c0563f25258445f7e85c496e2592939c2ceabaf3940480d2cd9d9374";
    hashRequest( "digest_by_name", "sha224", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_sha256 ) {  // 1.6.3
    const auto inputData = TestMessage;
    const auto expectedValue = "f4d54d32e3523357ff023903eaba2721e8c8cfc7702663782cb3e52faf2c56c002cc3096b5f2b6df870be66"
                               "5d0040e9963590eb02d03d166e52999cd1c430db1";
    hashRequest( "digest_by_name", "sha512", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_sha384 ) {  // 1.6.4
    const auto inputData = TestMessage;
    const auto expectedValue =
        "13e5716c7a7e678bce1b097dce6a1e10f0c60ced395e897f4fcd0146f20c78e3b60b5dd57fd74c62704b0af0ccfd3725";
    hashRequest( "digest_by_name", "sha384", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_sha512 ) {  // 1.6.5
    const auto inputData = TestMessage;
    const auto expectedValue = "f4d54d32e3523357ff023903eaba2721e8c8cfc7702663782cb3e52faf2c56c002cc3096b5f2b6df870be66"
                               "5d0040e9963590eb02d03d166e52999cd1c430db1";
    hashRequest( "digest_by_name", "sha512", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_md4 ) {  // 1.6.6
    const auto inputData = TestMessage;
    const auto expectedValue = "2292dda5d12d36a89dc8e4d3b54feb7c";
    hashRequest( "digest_by_name", "md4", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_dss1 ) {  // 1.6.7
    const auto inputData = TestMessage;
    const auto expectedValue = "f72017485fbf6423499baf9b240daa14f5f095a1";
    hashRequest( "digest_by_name", "dss1", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, digestByName_ripemd160 ) {  // 1.6.8
    const auto inputData = TestMessage;
    const auto expectedValue = "291850ad6a9a191487f01b5fbe19c215de1a5d67";
    hashRequest( "digest_by_name", "ripemd160", inputData, __LINE__ );

    checkUiBroadcast( expectedValue, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, getAllDigestNames ) {  // 1.7
    const auto expectedValue = "291850ad6a9a191487f01b5fbe19c215de1a5d67";
    hashRequest( "get_all_digest_names", "", "", __LINE__ );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.extensions.plugin.ui_broadcast" );
    ASSERT_FALSE( broadcast.empty() );
    JsonObject result, digestNames;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
    ASSERT_TRUE( json_get_arr( result, "value", digestNames ) );
    ASSERT_FALSE( digestNames.empty() );
    LOGGER_INFO( cc::j( digestNames ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, fromHexString ) {  // 1.8
    {                                               // check ok
        auto hexMessage = "DEADbeaf0123456789";
        const auto expectedValue = "22ab77d07e45cb0ba02e42f45f922657";
        hashRequest( "from_hex_string", "md5", hexMessage, __LINE__ );

        checkUiBroadcast( expectedValue, __LINE__ );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    }

    {  // check failure
        hashRequest( "from_hex_string", "md5", TestMessage, __LINE__ );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        // input data couldn't be transformed to binary
        checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
    }
}

TEST_F( LuadSecurityModuleTests, fromHexStringToBase64 ) {  // 1.8.1
    auto hexMessage = "mXOdcs1plinT1EPDFfI7_ffIY4oCredkCmQ6AEOzjQ4";
    const auto expectedSha256Value = "c393db3f0f5a680ffd1d2a38373b525ad5e4e0702c2ba9720502a9c0f0761f8c";
    hashRequest( "", "sha256", hexMessage, __LINE__ );

    checkUiBroadcast( expectedSha256Value, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    hashRequest( "from_hex_string", "base64", expectedSha256Value, __LINE__ );

    auto expectedBase64 = "w5PbPw9aaA/9HSo4NztSWtXk4HAsK6lyBQKpwPB2H4w=";
    checkUiBroadcast( expectedBase64, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, fromHexStringFails ) {  // 1.8.2
    hashRequest( "from_hex_string", "md5", TestMessage, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    // input data couldn't be transformed to binary
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );

    hashRequest( "from_hex_string", "md5", "0", __LINE__ );  // odd number of bytes

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    // input data couldn't be transformed to binary
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );

    hashRequest( "from_hex_string", "md5", "zzz", __LINE__ );  // non-hex characters

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    // input data couldn't be transformed to binary
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );

    hashRequest( "from_hex_string", "md5", "012", __LINE__ );  // odd number of bytes

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    // input data couldn't be transformed to binary
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_Encryption ) {  // 2.1.1
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "data" ] = "hello";
    auto& cipher = scriptParams[ "cipher" ];
    cipher = JSON_EMPTY_OBJ;
    cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
    checkResult( request, "\xE5\x13\xB8\x3E\x39\x20\x54\x0D\x4F\xF8\x23\x36\xB9\xBC\x7C\xF3", __LINE__ );

    cipher[ "key" ] = "17C63C37DE19CF486AC9151A";
    cipher[ "mode" ] = "cbc";
    checkResult( request, "\x7A\xF8\x7C\xC8\x17\x49\x78\x39\x85\x60\x21\x3B\x56\x23\xFB\x12", __LINE__ );

    cipher[ "key" ] = "17C63C37DE19CF48";
    cipher[ "mode" ] = "ofb";
    checkResult( request, "\xFE\x2D\xC2\x59\xDB", __LINE__ );

    cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
    cipher[ "mode" ] = "cbc";
    cipher[ "iv" ] = "17C63C37DE19CF48";
    checkResult( request, "\x9F\xAA\x18\x59\xA8\x75\x66\xC4\xCB\x09\x54\xF8\xAE\x81\x7F\x3E", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_BinaryDataEncryption ) {  // 2.1.2
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto expected = std::string(
        "\x29\x8e\x72\xcb\x66\x68\x86\x5b\xb7\x3b\xca\x6c\x75\xcd\x07\xf4\xf2\xb5\x49\xd9\xf1\xa3\x45\x21\x53\x15\xde"
        "\x06\x1b\xd2\x08\x5a\x0a\xf5\xd8\x60\x04\x25\x7a\x25\xff\xe5\x19\x1b\x81\xc1\x1a\x47\xe4\xf7\x72\xde\x61\x08"
        "\x8b\x3f\x35\xfe\x88\xb4\xe5\x7f\x7d\xaf\x68\x7b\xc7\x71\x5a\x9d\x6c\x93\x84\x31\x07\x3d\x49\x14\xa7\x26",
        80 );
    scriptParams[ "data" ] = std::string(
        "\x00\x00\x00\x00\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x54\x65\x73\x74\x20\x20"
        "\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        80 );
    auto& cipher = scriptParams[ "cipher" ];
    cipher = JSON_EMPTY_OBJ;
    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );

    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "mode" ] = "cbc";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );

    expected = std::string(
        "\x45\x34\x52\xe7\xf9\x2e\xda\x95\x83\x44\x93\x08\x35\xef\x9a\x6d\xfb\x69\x2d\xc3\x70\xb9\x04\x43\xac\x5c\xd6"
        "\x3f\xbb\x53\xad\xfa\x08\x81\x4c\xa7\xf8\xcf\x41\x71\x00\x32\x8e\x57\x0c\x3b\x86\xc9\x4d\x05\x70\x84\x49\xa3"
        "\x89\xe2\x9a\xe1\x04\x54\x36\xa0\x5b\xdd\xdc\x02\xc1\x61\xaf\x13\x25\xe8\x7e\x19\xb0\xf7\xd1\xce\x06\x8d",
        80 );
    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "mode" ] = "cbc";
    cipher[ "iv" ] = "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_EncryptLuaData ) {  // 2.1.3
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptLuaDataScript } } );
    auto expected = std::string(
        "\x45\x34\x52\xe7\xf9\x2e\xda\x95\x83\x44\x93\x08\x35\xef\x9a\x6d\xfb\x69\x2d\xc3\x70\xb9\x04\x43\xac\x5c\xd6"
        "\x3f\xbb\x53\xad\xfa\x08\x81\x4c\xa7\xf8\xcf\x41\x71\x00\x32\x8e\x57\x0c\x3b\x86\xc9\x4d\x05\x70\x84\x49\xa3"
        "\x89\xe2\x9a\xe1\x04\x54\x36\xa0\x5b\xdd\xdc\x02\xc1\x61\xaf\x13\x25\xe8\x7e\x19\xb0\xf7\xd1\xce\x06\x8d",
        80 );
    checkResult( request, expected, __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_Decryption ) {  // 2.2.1
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& cipher = scriptParams[ "cipher" ];
    cipher = JSON_EMPTY_OBJ;

    scriptParams[ "data" ] = "\xE5\x13\xB8\x3E\x39\x20\x54\x0D\x4F\xF8\x23\x36\xB9\xBC\x7C\xF3";
    cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
    checkResult( request, "hello", __LINE__ );

    scriptParams[ "data" ] = "\x7A\xF8\x7C\xC8\x17\x49\x78\x39\x85\x60\x21\x3B\x56\x23\xFB\x12";
    cipher[ "key" ] = "17C63C37DE19CF486AC9151A";
    cipher[ "mode" ] = "cbc";
    checkResult( request, "hello", __LINE__ );

    scriptParams[ "data" ] = "\xFE\x2D\xC2\x59\xDB";
    cipher[ "key" ] = "17C63C37DE19CF48";
    cipher[ "mode" ] = "ofb";
    checkResult( request, "hello", __LINE__ );

    scriptParams[ "data" ] = "\x9F\xAA\x18\x59\xA8\x75\x66\xC4\xCB\x09\x54\xF8\xAE\x81\x7F\x3E";
    cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
    cipher[ "mode" ] = "cbc";
    cipher[ "iv" ] = "17C63C37DE19CF48";
    checkResult( request, "hello", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_BinaryDataDecryption ) {  // 2.2.2
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    auto& scriptParams = request[ "params" ][ "scriptParams" ];
    scriptParams = JSON_EMPTY_OBJ;
    auto& cipher = scriptParams[ "cipher" ];
    cipher = JSON_EMPTY_OBJ;

    auto expected = std::string( "\x01\x00\x00\x00\x46\xae\x37\xe8\x35\x59\xa2\xfa\x67\x10\x59\x49\x70\x2d\xed\xe7\x00"
                                 "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
                                 32 );
    auto data = std::string( "\x34\xbc\x31\x4c\xd7\x47\x48\xa3\x28\x3c\x3b\x8d\xbd\x2d\xdb\x88\xa7\x51\xe7\xe0\xee\x35"
                             "\x79\x7f\x4e\xa9\xb3\x31\xed\x4c\xbb\x20",
                             32 );
    scriptParams[ "data" ] = data;
    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );

    scriptParams[ "data" ] = data;
    cipher[ "mode" ] = "cbc";
    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );

    scriptParams[ "data" ] = std::string( "\xcf\xa5\x74\x05\x48\xd5\x8d\x5c\x63\xf0\x4e\x18\x74\xa3\xce\x90\xe4\x48\x79"
                                          "\xa7\x5b\x08\xab\xde\x35\x2f\x4e\x52\x91\x27\x50\x06",
                                          32 );
    cipher[ "mode" ] = "cbc";
    cipher[ "key" ] = "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02";
    cipher[ "iv" ] = "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58";
    cipher[ "auto_padding" ] = false;
    checkResult( request, expected, __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_WrongKeySize ) {  // 2.3.1
    auto setScriptParams = []( Json& request ) {
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "data" ] = "DATA";
        auto& cipher = scriptParams[ "cipher" ];
        cipher = JSON_EMPTY_OBJ;
        cipher[ "key" ] = "17C63C37DE19CF486AC9151A073";
    };
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    setScriptParams( request );
    checkFailure( request, "Wrong key length", __LINE__ );

    request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    setScriptParams( request );
    checkFailure( request, "Wrong key length", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_UnknownMode ) {  // 2.3.3
    auto setScriptParams = []( Json& request ) {
        auto& scriptParams = request[ "params" ][ "scriptParams" ];
        scriptParams = JSON_EMPTY_OBJ;
        scriptParams[ "data" ] = "DATA";
        auto& cipher = scriptParams[ "cipher" ];
        cipher = JSON_EMPTY_OBJ;
        cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
        cipher[ "mode" ] = "wrap";
    };
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    setScriptParams( request );
    checkFailure( request, "Mode WRAP not found", __LINE__ );

    request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    setScriptParams( request );
    checkFailure( request, "Mode WRAP not found", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_BadCipher ) {  // 2.3.4
    auto setScriptParams = []( Json& request ) {
        auto& params = request[ "params" ][ "scriptParams" ];
        params = JSON_EMPTY_OBJ;
        params[ "data" ] = "DATA";
        params[ "cipher" ] = "bad cipher";
        params[ "data" ] = "hello";
    };
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    setScriptParams( request );
    checkFailure( request, "bad argument #1 to 'encrypt' (table expected, got string)", __LINE__ );

    request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    setScriptParams( request );
    checkFailure( request, "bad argument #1 to 'decrypt' (table expected, got string)", __LINE__ );
}

TEST_F( LuadSecurityModuleTests, SecurityTest_AesCipher_NoData ) {  // 2.3.5
    auto setScriptParams = []( Json& request ) {
        auto& params = request[ "params" ][ "scriptParams" ];
        params = JSON_EMPTY_OBJ;
        auto& cipher = params[ "cipher" ];
        cipher = JSON_EMPTY_OBJ;
        cipher[ "key" ] = "17C63C37DE19CF486AC9151A073C7843";
    };
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", encryptScript } } );
    setScriptParams( request );
    checkFailure( request, "security.encrypt: fail to get a parameter (NativeString) at 2 index", __LINE__ );

    request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", decryptScript } } );
    setScriptParams( request );
    checkFailure( request, "security.decrypt: fail to get a parameter (NativeString) at 2 index", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
