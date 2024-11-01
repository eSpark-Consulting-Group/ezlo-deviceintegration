#pragma once

#include "base_utils/non_copyable.h"
#include "crypto_utils/crypto_key_aes256.h"
#include "crypto_utils/cipher.h"
#include "crypto_utils/digests.h"

#include <memory>
#include <random>
#include <iosfwd>
#include <map>
#include <tuple>
#include <set>

extern "C" {
#include <openssl/ssl.h>
#include <openssl/evp.h>
}

namespace HubPlatform {

namespace UT {
class CryptoUtilsTestSuite;
}  // namespace UT

namespace CryptoUtils {

class Cipher;

class Crypto : private common::NonCopyable {
    friend class UT::CryptoUtilsTestSuite;

public:
    static Crypto& getInstance();
    static Digests& digests();

public:
    using EvpCipherCtxPtr = std::unique_ptr< EVP_CIPHER_CTX, decltype( &EVP_CIPHER_CTX_free ) >;

    std::string encrypt( const Cipher& cipher, const std::string& data );
    std::string decrypt( const Cipher& cipher, const std::string& data );

    std::string encryptAes256( const char* text,
                               int textLength,
                               const unsigned char* iv,
                               const CryptoUtils::KeyAes256& key );

    std::string decryptAes256( const char* text,
                               int textLength,
                               const unsigned char* iv,
                               const CryptoUtils::KeyAes256& key );

    template < typename KeyType >
    std::string generateIv() {
        static constexpr unsigned int keySize = KeyType::ms_ivSize;
        unsigned char data[ keySize ];
        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_int_distribution< unsigned char > d( 0x00, 0xFF );
        for( auto& c : data )
            c = d( gen );

        return std::string{ data, data + keySize };
    }

    std::string toBase64( const std::string& );
    std::string fromBase64( const std::string& );

private:
    Crypto();
    ~Crypto();

    void addCipher( const EVP_CIPHER* );

    const unsigned char* toBytePointer( const std::string& str );

private:
    std::set< const EVP_CIPHER* > m_addedCiphers;
};

}  // namespace CryptoUtils
}  // namespace HubPlatform
