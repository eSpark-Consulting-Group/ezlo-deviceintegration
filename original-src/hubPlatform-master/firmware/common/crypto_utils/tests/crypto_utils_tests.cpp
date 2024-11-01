#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <random>
#include <fstream>
#include <sstream>
#include <tiny/tiny_utils.h>

#include "json_utils/json_utils.h"

#include "crypto_utils/crypto_utils.h"
#include "crypto_utils/crypto_key_aes256.h"
#include "crypto_utils/aes_cipher.h"
#include "crypto_utils/crypto_exception.h"
#include "base_utils/utils.h"

#include "common_utils/cpp14.h"

/*

TESTPLAN

DONE    1. AES 256 tests
DONE        1.1. More than 16 bytes
DONE        1.2. 16 bytes
DONE        1.3. Less than 16 bytes
DONE        1.4. Long text
DONE        1.5. Json
DONE        1.6. Provision info example
DONE        1.7. Pre-encrypted zero padded unaligned text
DONE        1.8. Pre-encrypted zero padded aligned text
DONE        1.9. Pre-encrypted PKCS padded unaligned text
DONE        1.10. Pre-encrypted PKCS padded aligned text
DONE    2. Hash tests
DONE        2.1. Test md5 checksum
DONE        2.2. sha1
DONE    3. AES cipher tests
DONE        3.1. encryption
DONE            3.1.1 different cipher types encryption
DONE            3.1.2 different data length encryption
DONE        3.2. decryption
DONE            3.2.1 different cipher types decryption
DONE            3.2.2 different data length decryption
DONE        3.3. random IV
DONE        3.4. cipher validation
DONE        3.5. failures
DONE            3.5.1 encryption failure
DONE            3.5.2 decryption failure
DONE        3.6. no auto padding
DONE            3.5.1 string data
DONE            3.5.2 binary data
DONE            3.5.3 long binary data

*/

namespace HubPlatform {
namespace UT {

class CryptoUtilsTestSuite : public ::testing::Test {
public:
    using Crypto = CryptoUtils::Crypto;
    using Key = CryptoUtils::KeyAes256;

protected:
    static const unsigned char* generateAes256KeyBytes() {
        static constexpr unsigned int KeySize = Key::ms_keySize;
        static unsigned char bytes[ KeySize ];

        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_int_distribution< unsigned char > d( 0x00, 0xFF );

        for( auto& c : bytes )
            c = d( gen );

        return bytes;
    }

    const unsigned char* generateIvBytes() {
        static std::string iv = m_crypto.generateIv< Key >();
        return reinterpret_cast< const unsigned char* >( iv.c_str() );
    }

    std::string generateIv() {
        return m_crypto.generateIv< Key >();
    }

    void checkAesEncryption( CryptoUtils::AesCipher::Mode mode,
                             int keySize,
                             const std::string& expected,
                             int codeLine ) {
        checkAesEncryption( std::string( "hello", 5 ), mode, keySize, expected, codeLine );
    }

    void checkAesEncryption( const std::string& data,
                             CryptoUtils::AesCipher::Mode mode,
                             int keySize,
                             const std::string& expected,
                             int codeLine ) {
        using namespace CryptoUtils;
        auto codeLineStr = "code line: " + std::to_string( codeLine );
        auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
        key.resize( keySize );
        auto encryptedText = m_crypto.encrypt( AesCipher( mode, key ), data );
        EXPECT_EQ( expected, m_crypto.toBase64( encryptedText ) ) << codeLineStr;
    }

    void checkAesDecryption( CryptoUtils::AesCipher::Mode mode, int keySize, const std::string& data, int codeLine ) {
        checkAesDecryption( std::string( "hello", 5 ), mode, keySize, data, codeLine );
    }

    void checkAesDecryption( const std::string& expected,
                             CryptoUtils::AesCipher::Mode mode,
                             int keySize,
                             const std::string& data,
                             int codeLine ) {
        using namespace CryptoUtils;
        auto codeLineStr = "code line: " + std::to_string( codeLine );
        auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
        key.resize( keySize );
        auto decryptedText = m_crypto.decrypt( AesCipher( mode, key ), m_crypto.fromBase64( data ) );
        EXPECT_EQ( expected, decryptedText ) << codeLineStr;
    }

    void checkAesIvChanging( CryptoUtils::AesCipher::Mode mode, const std::string& iv, int codeLine ) {
        using namespace CryptoUtils;
        auto codeLineStr = "code line: " + std::to_string( codeLine );
        auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
        AesCipher aesCipher( mode, key, std::string( iv.c_str(), iv.size() ) );
        auto expected = std::string( "hello", 5 );
        auto encryptedText = m_crypto.encrypt( aesCipher, expected );
        auto decryptedText = m_crypto.decrypt( aesCipher, encryptedText );
        EXPECT_EQ( expected, decryptedText ) << codeLineStr;
    }

public:
    CryptoUtilsTestSuite()
    : m_crypto( Crypto::getInstance() )
    , m_key( generateAes256KeyBytes() ) {}

protected:
    Crypto& m_crypto;
    Key m_key;
};

TEST_F( CryptoUtilsTestSuite, Aes256_MoreThan16bytes /*1.1*/ ) {
    const unsigned char* iv = generateIvBytes();
    std::string text = "Hello World! This is a test message longer than 16 characters";
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_16bytes /*1.2*/ ) {
    const unsigned char* iv = generateIvBytes();
    std::string text = "Hello World! 123";
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_LessThan16bytes /*1.3*/ ) {
    const unsigned char* iv = generateIvBytes();
    std::string text = "12345";
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_LongText /*1.4*/ ) {
    const unsigned char* iv = generateIvBytes();
    std::string text =
        "P1ILydQsgzImvqsiHTl6aIKgBduToSjkMtFjk9sz9goUtZrYGbQwmP5hqABVGw4T5k0yBHHkvYsJn5zEVmSTWwGXmez96XMM1T4h";
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_Json /*1.5*/ ) {
    const unsigned char* iv = generateIvBytes();
    JsonObject originalJson{ { "a", 1 }, { "b", "2" }, { "c", { "d", "e" } } };
    std::string text = originalJson.dump();
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );
    JsonObject json;

    EXPECT_EQ( text, decryptedText );
    EXPECT_TRUE( parseJson( decryptedText, json ) );
    EXPECT_EQ( originalJson, json );
}

TEST_F( CryptoUtilsTestSuite, Aes256_ProvisionInfoExample /*1.6*/ ) {
    const unsigned char* iv = generateIvBytes();
    JsonObject originalJson{ { "id", 12345678 },
                             { "uuid", "dfa943a4-3c6c-4003-bca5-a62c71f58f9f" },
                             { "provision_endpoint", "https://provision.mios.com" },
                             { "provision_key", "abcdefghijklmnoqrstuvwxyz" },
                             { "cloud_endpoints", "https://cloud.dev.mios.com" },
                             { "default_wifi_ssid", "MIOS" },
                             { "default_wifi_password", "boringmios" },
                             { "default_ssh_password", "some-password-in-linux-format" } };
    std::string text = originalJson.dump();
    std::string encryptedText = m_crypto.encryptAes256( text.c_str(), text.size(), iv, m_key );
    std::string decryptedText = m_crypto.decryptAes256( encryptedText.c_str(), encryptedText.size(), iv, m_key );
    JsonObject json;

    EXPECT_EQ( text, decryptedText );
    EXPECT_TRUE( parseJson( decryptedText, json ) );
    EXPECT_EQ( originalJson, json );
}

TEST_F( CryptoUtilsTestSuite, Aes256_PreEncryptedZeroPaddedUnallignedText /*1.7*/ ) {
    std::string iv = "1234567890ABCDEF";
    CryptoUtils::KeyAes256 key( reinterpret_cast< const unsigned char* >(
        "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" ) );
    std::string text = "abcd";
    std::string encryptedText = "\132\272\226\063\263\112\322\055\373\020\043\204\262\275\205\301";
    std::string decryptedText = m_crypto.decryptAes256(
        encryptedText.c_str(), encryptedText.size(), reinterpret_cast< const unsigned char* >( iv.c_str() ), key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_PreEncryptedZeroPaddedAllignedText /*1.8*/ ) {
    std::string iv = "1234567890ABCDEF";
    CryptoUtils::KeyAes256 key( reinterpret_cast< const unsigned char* >(
        "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" ) );
    std::string text( "abcdefghijklmnop", 16 );
    std::string encryptedText = "\034\176\264\307\327\012\245\273\351\325\244\037\047\007\043\054";
    std::string decryptedText = m_crypto.decryptAes256(
        encryptedText.c_str(), encryptedText.size(), reinterpret_cast< const unsigned char* >( iv.c_str() ), key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_PreEncryptedPKCSPaddedUnallignedText /*1.9*/ ) {
    std::string iv = "1234567890ABCDEF";
    CryptoUtils::KeyAes256 key( reinterpret_cast< const unsigned char* >(
        "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" ) );
    std::string text = "abcd";
    std::string encryptedText = "\357\076\376\300\176\162\172\267\201\163\014\333\165\220\057\361";
    std::string decryptedText = m_crypto.decryptAes256(
        encryptedText.c_str(), encryptedText.size(), reinterpret_cast< const unsigned char* >( iv.c_str() ), key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, Aes256_PreEncryptedPKCSPaddedAllignedText /*1.10*/ ) {
    std::string iv = "1234567890ABCDEF";
    CryptoUtils::KeyAes256 key( reinterpret_cast< const unsigned char* >(
        "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" ) );
    std::string text = "abcdefghijklmnop";
    std::string encryptedText = "\034\176\264\307\327\012\245\273\351\325\244\037\047\007\043\054\071\257\222\264\001"
                                "\212\370\226\100\232\107\227\206\072\321\113";
    std::string decryptedText = m_crypto.decryptAes256(
        encryptedText.c_str(), encryptedText.size(), reinterpret_cast< const unsigned char* >( iv.c_str() ), key );

    EXPECT_EQ( text, decryptedText );
}

TEST_F( CryptoUtilsTestSuite, AesEncryption /*3.1.1*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
    EXPECT_EQ( std::string( "\xe5\x13\xb8\x3e\x39\x20\x54\x0d\x4f\xf8\x23\x36\xb9\xbc\x7c\xf3", 16 ),
               m_crypto.encrypt( AesCipher( AesCipher::Mode::CBC, key ), std::string( "hello", 5 ) ) );

    checkAesEncryption( AesCipher::Mode::CBC, 16, "kqNYzjH/fzM9/znP3GAA3g==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB1, 16, "pCyZH7c=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB8, 16, "/jTlJEY=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CTR, 16, "/i3CWds=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::ECB, 16, "kqNYzjH/fzM9/znP3GAA3g==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::OFB, 16, "/i3CWds=", __LINE__ );

    checkAesEncryption( AesCipher::Mode::CBC, 24, "evh8yBdJeDmFYCE7ViP7Eg==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB1, 24, "Q03QKAk=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB8, 24, "MJ/W5uU=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CTR, 24, "MErzUcE=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::ECB, 24, "evh8yBdJeDmFYCE7ViP7Eg==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::OFB, 24, "MErzUcE=", __LINE__ );

    checkAesEncryption( AesCipher::Mode::CBC, 32, "5RO4PjkgVA1P+CM2ubx88w==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB1, 32, "XP0iK8Y=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CFB8, 32, "SQ8ZQkI=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::CTR, 32, "SbLjuOE=", __LINE__ );
    checkAesEncryption( AesCipher::Mode::ECB, 32, "5RO4PjkgVA1P+CM2ubx88w==", __LINE__ );
    checkAesEncryption( AesCipher::Mode::OFB, 32, "SbLjuOE=", __LINE__ );
}

TEST_F( CryptoUtilsTestSuite, AesEncryptionDifferentDataLength /*3.1.2*/ ) {
    using namespace CryptoUtils;
    std::string data = "hellohellohelloh";
    checkAesEncryption( data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnU5RK8Xx1DtqeopsYMXepkI=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQ==", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bQ==", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6zw==", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSnbjKXRsIQx3gKpgJLvy/Edg=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6zw==", __LINE__ );

    data = "hellohellohellohello";
    checkAesEncryption( data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnTbz49FE3jI8qLMc5kF9JII=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQhA+Jg=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bTQE8LU=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6z9+vV+s=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSndp9245lXu0X/eQSrw25Iq4=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6z1QXFoo=", __LINE__ );

    data = "hellohellohellohellohellohellohello";
    checkAesEncryption(
        data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnbXhqAqD+grOPfndrzXvY+NUil5zijraoyNVCewjsxxh", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQhA+Jjt1l8R9aDFUI6eByky1WQ=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bTQE8LWT8Z7RZEGjCbEdEzy4wXY=", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6z9+vV+vt9bZ5jWQN+JKDPkjmFuk=", __LINE__ );
    checkAesEncryption(
        data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSnSCBnjjXgdspYqpv0QZUNPiTr42prLKdYpjWf8Z7WOIq", __LINE__ );
    checkAesEncryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6z1QXForjiT1ol9bdV4RIDzW7+AE=", __LINE__ );
}

TEST_F( CryptoUtilsTestSuite, AesDecryption /*3.2.1*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
    EXPECT_EQ(
        std::string( "hello", 5 ),
        m_crypto.decrypt( AesCipher( AesCipher::Mode::CBC, key ),
                          std::string( "\xe5\x13\xb8\x3e\x39\x20\x54\x0d\x4f\xf8\x23\x36\xb9\xbc\x7c\xf3", 16 ) ) );

    checkAesDecryption( AesCipher::Mode::CBC, 16, "kqNYzjH/fzM9/znP3GAA3g==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB1, 16, "pCyZH7c=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB8, 16, "/jTlJEY=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CTR, 16, "/i3CWds=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::ECB, 16, "kqNYzjH/fzM9/znP3GAA3g==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::OFB, 16, "/i3CWds=", __LINE__ );

    checkAesDecryption( AesCipher::Mode::CBC, 24, "evh8yBdJeDmFYCE7ViP7Eg==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB1, 24, "Q03QKAk=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB8, 24, "MJ/W5uU=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CTR, 24, "MErzUcE=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::ECB, 24, "evh8yBdJeDmFYCE7ViP7Eg==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::OFB, 24, "MErzUcE=", __LINE__ );

    checkAesDecryption( AesCipher::Mode::CBC, 32, "5RO4PjkgVA1P+CM2ubx88w==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB1, 32, "XP0iK8Y=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CFB8, 32, "SQ8ZQkI=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::CTR, 32, "SbLjuOE=", __LINE__ );
    checkAesDecryption( AesCipher::Mode::ECB, 32, "5RO4PjkgVA1P+CM2ubx88w==", __LINE__ );
    checkAesDecryption( AesCipher::Mode::OFB, 32, "SbLjuOE=", __LINE__ );
}

TEST_F( CryptoUtilsTestSuite, AesDecryptionDifferentDataLength /*3.2.1*/ ) {
    using namespace CryptoUtils;
    std::string data = "hellohellohelloh";
    checkAesDecryption( data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnU5RK8Xx1DtqeopsYMXepkI=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQ==", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bQ==", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6zw==", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSnbjKXRsIQx3gKpgJLvy/Edg=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6zw==", __LINE__ );

    data = "hellohellohellohello";
    checkAesDecryption( data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnTbz49FE3jI8qLMc5kF9JII=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQhA+Jg=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bTQE8LU=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6z9+vV+s=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSndp9245lXu0X/eQSrw25Iq4=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6z1QXFoo=", __LINE__ );

    data = "hellohellohellohellohellohellohello";
    checkAesDecryption(
        data, AesCipher::Mode::CBC, 32, "Am0Yiu9zlYwGKqFnkfxSnbXhqAqD+grOPfndrzXvY+NUil5zijraoyNVCewjsxxh", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB1, 32, "XP0iK8YNKaN6xDU2lKMhgQhA+Jjt1l8R9aDFUI6eByky1WQ=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CFB8, 32, "SQ8ZQkIw40Gf7sUTgRq5bTQE8LWT8Z7RZEGjCbEdEzy4wXY=", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::CTR, 32, "SbLjuOGbQFxW8sg+nWo6z9+vV+vt9bZ5jWQN+JKDPkjmFuk=", __LINE__ );
    checkAesDecryption(
        data, AesCipher::Mode::ECB, 32, "Am0Yiu9zlYwGKqFnkfxSnSCBnjjXgdspYqpv0QZUNPiTr42prLKdYpjWf8Z7WOIq", __LINE__ );
    checkAesDecryption( data, AesCipher::Mode::OFB, 32, "SbLjuOGbQFxW8sg+nWo6z1QXForjiT1ol9bdV4RIDzW7+AE=", __LINE__ );
}

TEST_F( CryptoUtilsTestSuite, AesRandomIv /*3.3*/ ) {
    using namespace CryptoUtils;
    checkAesIvChanging( AesCipher::Mode::CBC, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CBC, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CBC, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CBC, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CBC, "234g3g3g342fdf", __LINE__ );

    checkAesIvChanging( AesCipher::Mode::CFB1, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB1, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB1, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB1, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB1, "234g3g3g342fdf", __LINE__ );

    checkAesIvChanging( AesCipher::Mode::CFB8, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB8, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB8, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB8, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CFB8, "234g3g3g342fdf", __LINE__ );

    checkAesIvChanging( AesCipher::Mode::CTR, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CTR, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CTR, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CTR, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::CTR, "234g3g3g342fdf", __LINE__ );

    checkAesIvChanging( AesCipher::Mode::ECB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::ECB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::ECB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::ECB, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::ECB, "234g3g3g342fdf", __LINE__ );

    checkAesIvChanging( AesCipher::Mode::OFB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::OFB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::OFB, generateIv(), __LINE__ );
    checkAesIvChanging( AesCipher::Mode::OFB, "sdfsd", __LINE__ );
    checkAesIvChanging( AesCipher::Mode::OFB, "234g3g3g342fdf", __LINE__ );
}

TEST_F( CryptoUtilsTestSuite, AesCipherValidation /*3.4*/ ) {
    using namespace CryptoUtils;
    EXPECT_NO_THROW( AesCipher( AesCipher::Mode::CBC,
                                std::string( "17C63C37DE19CF486AC9151A073C7843", 32 ),
                                std::string( "17C63C37DE19CF48", 16 ) ) );
    EXPECT_THROW( AesCipher( AesCipher::Mode::CBC, std::string( "17C63C37DE19CF486AC9151A073C78438", 33 ) ),
                  CryptoException );
    EXPECT_THROW( AesCipher( AesCipher::Mode::CBC, std::string( "17C63C37DE19CF486AC9151A073C", 28 ) ),
                  CryptoException );
}

TEST_F( CryptoUtilsTestSuite, AesEncryptionFailure /*3.5.1*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
    auto iv = std::string( "17C63C37DE19CF48", 16 );
    EXPECT_THROW( m_crypto.decrypt( AesCipher( AesCipher::Mode::CBC, key, iv, false ), "not padded data size" ),
                  CryptoException );
}

TEST_F( CryptoUtilsTestSuite, AesDecryptionFailure /*3.5.2*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "17C63C37DE19CF486AC9151A073C7843", 32 );
    EXPECT_THROW( m_crypto.decrypt( AesCipher( AesCipher::Mode::CBC, key ), std::string( "wrong data", 10 ) ),
                  CryptoException );
}

TEST_F( CryptoUtilsTestSuite, AesNoAutoPaddingStringData /*3.6.1*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02", 16 );
    auto iv = std::string( "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58", 16 );
    auto expected = std::string( "\n\xee;\x9b\xf9\xa9\x1b\xc9\x91gq\t\x01\x05\x45o", 16 );
    auto data = std::string( "hellohellohelloh", 16 );
    auto cipher = AesCipher( AesCipher::Mode::CBC, key, iv, /*enablePadding*/ false );
    auto encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    auto decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
    encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
}

TEST_F( CryptoUtilsTestSuite, AesNoAutoPaddingBinaryData /*3.6.2*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02", 16 );
    auto iv = std::string( "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58", 16 );
    auto expected = std::string( "\x45\x34\x52\xe7\xf9\x2e\xda\x95\x83\x44\x93\x08\x35\xef\x9a\x6d", 16 );
    auto data = std::string( "\x00\x00\x00\x00\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31", 16 );
    auto cipher = AesCipher( AesCipher::Mode::CBC, key, iv, /*enablePadding*/ false );
    auto encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    auto decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
    encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
}

TEST_F( CryptoUtilsTestSuite, AesNoAutoPaddingLongBinaryData /*3.6.3*/ ) {
    using namespace CryptoUtils;
    auto key = std::string( "\x09\x76\x28\x34\x3f\xe9\x9e\x23\x76\x5c\x15\x13\xac\xcf\x8b\x02", 16 );
    auto iv = std::string( "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58", 16 );
    auto expected = std::string(
        "\x45\x34\x52\xe7\xf9\x2e\xda\x95\x83\x44\x93\x08\x35\xef\x9a\x6d\xfb\x69\x2d\xc3\x70\xb9\x04\x43\xac\x5c\xd6"
        "\x3f\xbb\x53\xad\xfa\x08\x81\x4c\xa7\xf8\xcf\x41\x71\x00\x32\x8e\x57\x0c\x3b\x86\xc9\x4d\x05\x70\x84\x49\xa3"
        "\x89\xe2\x9a\xe1\x04\x54\x36\xa0\x5b\xdd\xdc\x02\xc1\x61\xaf\x13\x25\xe8\x7e\x19\xb0\xf7\xd1\xce\x06\x8d",
        80 );
    auto data = std::string(
        "\x00\x00\x00\x00\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x54\x65\x73\x74\x20\x20"
        "\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        80 );
    auto cipher = AesCipher( AesCipher::Mode::CBC, key, iv, /*enablePadding*/ false );
    auto encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    auto decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
    encryptedText = m_crypto.encrypt( cipher, data );
    EXPECT_EQ( encryptedText, expected );
    decryptedText = m_crypto.decrypt( cipher, encryptedText );
    EXPECT_EQ( decryptedText, data );
}

}  // namespace UT
}  // namespace HubPlatform
