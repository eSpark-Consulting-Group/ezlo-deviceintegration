#include "crypto_utils/aes_cipher.h"
#include "crypto_utils/crypto_exception.h"

#include <map>
#include <cstring>

extern "C" {
#include <openssl/ssl.h>
#include <openssl/evp.h>
}

namespace HubPlatform {
namespace CryptoUtils {

AesCipher::AesCipher( AesCipher::Mode mode, std::string key )
: AesCipher( mode, std::move( key ), {} ) {}

AesCipher::AesCipher( AesCipher::Mode mode, std::string key, std::string iv, bool enablePadding )
: m_mode( mode )
, m_key( std::move( key ) )
, m_iv( std::move( iv ) )
, m_isPaddingEnabled( enablePadding ) {
    auto size = m_key.size();
    if( ( size != 16 && size != 24 && size != 32 ) ) {
        throw CryptoException( "Wrong key length: " + std::to_string( size ) +
                               " for AES cipher type: " + toString( mode ) + ". Possible lengths are 16, 24, 32" );
    }
}

const EVP_CIPHER* AesCipher::getCipher() const {
    return getCipher( m_mode, m_key.size() );
}

const std::string& AesCipher::getKey() const {
    return m_key;
}

const std::string& AesCipher::getIV() const {
    return m_iv;
}

bool AesCipher::isPaddingEnabled() const {
    return m_isPaddingEnabled;
}

std::size_t AesCipher::getEncryptedDataSize( std::size_t inputDataSize ) const {
    if( m_mode == Mode::CBC || m_mode == Mode::ECB ) {
        const std::size_t paddingSize = 16;
        std::size_t encryptedDataSize = inputDataSize;
        if( isPaddingEnabled() ) {
            if( encryptedDataSize % paddingSize == 0 ) {
                encryptedDataSize += paddingSize;
            }
            else {
                const size_t nonPaddedSize = inputDataSize;
                const size_t numPaddingElements = ( paddingSize - nonPaddedSize % paddingSize ) % paddingSize;
                encryptedDataSize = nonPaddedSize + numPaddingElements;
            }
        }
        else {
            if( encryptedDataSize % paddingSize != 0 ) {
                throw CryptoException( "Data is not padded. Please enable auto padding" );
            }
        }
        return encryptedDataSize;
    }
    return inputDataSize;
}

std::string AesCipher::toString( AesCipher::Mode mode ) {
    static std::map< Mode, std::string > typeToStr = {
        { Mode::ECB, "EBC" },   { Mode::CBC, "CBC" }, { Mode::CFB1, "CFB1" },
        { Mode::CFB8, "CFB8" }, { Mode::OFB, "OFB" }, { Mode::CTR, "CTR" },
    };

    auto it = typeToStr.find( mode );
    if( it != typeToStr.end() ) {
        return it->second;
    }
    return std::to_string( static_cast< int >( mode ) );
}

AesCipher::Mode AesCipher::toMode( std::string mode ) {
    static std::map< std::string, Mode > strToType = {
        { "EBC", Mode::ECB },   { "CBC", Mode::CBC }, { "CFB1", Mode::CFB1 },
        { "CFB8", Mode::CFB8 }, { "OFB", Mode::OFB }, { "CTR", Mode::CTR },
    };

    for( auto& c : mode )
        c = toupper( c );
    auto it = strToType.find( mode );
    if( it != strToType.end() ) {
        return it->second;
    }
    throw CryptoException( "Mode " + mode + " not found" );
}

const EVP_CIPHER* AesCipher::getCipher( AesCipher::Mode type, std::size_t keySize ) const {
    using CipherKey = std::tuple< Mode, std::size_t >;
    static std::map< CipherKey, const EVP_CIPHER* > ciphers = {
        { CipherKey{ Mode::ECB, 16 }, EVP_aes_128_ecb() },   { CipherKey{ Mode::CBC, 16 }, EVP_aes_128_cbc() },
        { CipherKey{ Mode::CFB1, 16 }, EVP_aes_128_cfb1() }, { CipherKey{ Mode::CFB8, 16 }, EVP_aes_128_cfb8() },
        { CipherKey{ Mode::OFB, 16 }, EVP_aes_128_ofb() },   { CipherKey{ Mode::CTR, 16 }, EVP_aes_128_ctr() },
        { CipherKey{ Mode::ECB, 24 }, EVP_aes_192_ecb() },   { CipherKey{ Mode::CBC, 24 }, EVP_aes_192_cbc() },
        { CipherKey{ Mode::CFB1, 24 }, EVP_aes_192_cfb1() }, { CipherKey{ Mode::CFB8, 24 }, EVP_aes_192_cfb8() },
        { CipherKey{ Mode::OFB, 24 }, EVP_aes_192_ofb() },   { CipherKey{ Mode::CTR, 24 }, EVP_aes_192_ctr() },
        { CipherKey{ Mode::ECB, 32 }, EVP_aes_256_ecb() },   { CipherKey{ Mode::CBC, 32 }, EVP_aes_256_cbc() },
        { CipherKey{ Mode::CFB1, 32 }, EVP_aes_256_cfb1() }, { CipherKey{ Mode::CFB8, 32 }, EVP_aes_256_cfb8() },
        { CipherKey{ Mode::OFB, 32 }, EVP_aes_256_ofb() },   { CipherKey{ Mode::CTR, 32 }, EVP_aes_256_ctr() },
    };

    auto it = ciphers.find( CipherKey{ type, keySize } );
    if( it != ciphers.end() ) {
        return it->second;
    }
    throw CryptoException( "Cipher not found for " + std::to_string( keySize * 8 ) + "-" + toString( type ) );
}

}  // namespace CryptoUtils
}  // namespace HubPlatform
