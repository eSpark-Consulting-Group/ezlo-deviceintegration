#include "crypto_utils/crypto_utils.h"
#include "crypto_utils/crypto_key_aes256.h"
#include "crypto_utils/crypto_exception.h"
#include "tiny/tiny_utils.h"
#include <memory>

namespace HubPlatform {
namespace CryptoUtils {

Crypto::Crypto() {
    addCipher( EVP_aes_256_cbc() );
}

Crypto::~Crypto() {
    EVP_cleanup();
}

void Crypto::addCipher( const EVP_CIPHER* cipher ) {
    auto it = m_addedCiphers.find( cipher );
    if( it == m_addedCiphers.end() ) {
        m_addedCiphers.insert( cipher );
    }
}

const unsigned char* Crypto::toBytePointer( const std::string& str ) {
    return reinterpret_cast< const unsigned char* >( str.data() );
}

Crypto& Crypto::getInstance() {
    static Crypto instance;
    return instance;
}

Digests& Crypto::digests() {
    return Digests::getInstance();
}

std::string Crypto::encrypt( const Cipher& cipher, const std::string& data ) {
    int rc;
    addCipher( cipher.getCipher() );
    EvpCipherCtxPtr ctx( EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free );
    EVP_CIPHER_CTX_init( ctx.get() );

    rc = EVP_CipherInit_ex( ctx.get(),
                            cipher.getCipher(),
                            nullptr,
                            toBytePointer( cipher.getKey() ),
                            toBytePointer( cipher.getIV() ),
                            1 );  // 1 - encrypt
    if( rc != 1 ) {
        throw CryptoException( "Encrypt cipher creating failure" );
    }

    std::size_t encryptedDataSize = cipher.getEncryptedDataSize( data.size() );
    if( !cipher.isPaddingEnabled() ) {
        EVP_CIPHER_CTX_set_padding( ctx.get(), 0 );
    }

    std::string encryptedData( encryptedDataSize, 0 );
    int cipherSize = 0;
    rc = EVP_CipherUpdate(
        ctx.get(), (unsigned char*) encryptedData.data(), &cipherSize, toBytePointer( data ), data.size() );
    if( rc != 1 ) {
        throw CryptoException( "Encryption EVP_CipherUpdate failed!" );
    }

    int cipherSizeFinal = 0;
    rc = EVP_CipherFinal_ex( ctx.get(), (unsigned char*) encryptedData.data() + cipherSize, &cipherSizeFinal );
    if( rc != 1 ) {
        throw CryptoException( "Encryption EVP_CipherFinal_ex failed!" );
    }
    encryptedData.resize( cipherSize + cipherSizeFinal );
    return encryptedData;
}

std::string Crypto::decrypt( const Cipher& cipher, const std::string& data ) {
    int rc;
    addCipher( cipher.getCipher() );
    EvpCipherCtxPtr ctx( EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free );
    EVP_CIPHER_CTX_init( ctx.get() );

    rc = EVP_CipherInit_ex( ctx.get(),
                            cipher.getCipher(),
                            nullptr,
                            toBytePointer( cipher.getKey() ),
                            toBytePointer( cipher.getIV() ),
                            0 );  // 0 - decrypt
    if( rc != 1 ) {
        throw CryptoException( "Decrypt cipher creating failure" );
    }

    if( !cipher.isPaddingEnabled() ) {
        EVP_CIPHER_CTX_set_padding( ctx.get(), 0 );
    }

    std::string decryptedData( data.size(), 0 );
    int cipherSize = 0;
    rc = EVP_CipherUpdate(
        ctx.get(), (unsigned char*) decryptedData.data(), &cipherSize, toBytePointer( data ), data.size() );
    if( rc != 1 ) {
        throw CryptoException( "Decryption EVP_CipherUpdate failed!" );
    }

    int cipherSizeFinal;
    rc = EVP_CipherFinal_ex( ctx.get(), (unsigned char*) decryptedData.data() + cipherSize, &cipherSizeFinal );
    if( rc != 1 ) {
        throw CryptoException( "Decryption EVP_CipherFinal_ex failed!" );
    }
    decryptedData.resize( cipherSize + cipherSizeFinal );
    return decryptedData;
}

std::string Crypto::encryptAes256( const char* text,
                                   int textLength,
                                   const unsigned char* iv,
                                   const CryptoUtils::KeyAes256& key ) {
    int rc;

    EvpCipherCtxPtr ctx( EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free );
    EVP_CIPHER_CTX_init( ctx.get() );
    EVP_CIPHER_CTX_set_padding( ctx.get(), 1 );

    rc = EVP_CipherInit_ex( ctx.get(), EVP_aes_256_cbc(), nullptr, key.rawKey(), iv, 1 );  // 1 - encrypt
    if( rc != 1 )
        throw CryptoException( "EVP_CipherInit_ex failed!" );

    int cipherSize = textLength;
    auto cipherData = std::unique_ptr< unsigned char[], std::default_delete< unsigned char[] > >(
        new unsigned char[ cipherSize + 1u + CryptoUtils::KeyAes256::ms_ivSize ] );

    rc = EVP_CipherUpdate(
        ctx.get(), cipherData.get(), &cipherSize, reinterpret_cast< const unsigned char* >( text ), textLength );
    if( rc != 1 )
        throw CryptoException( "EVP_CipherUpdate failed!" );

    int cipherSizeFinal;
    rc = EVP_CipherFinal_ex( ctx.get(), cipherData.get() + cipherSize, &cipherSizeFinal );
    if( rc != 1 )
        throw CryptoException( "EVP_CipherFinal_ex failed!" );

    std::string cipherDataStr{ cipherData.get(), cipherData.get() + cipherSize + cipherSizeFinal };
    return cipherDataStr;
}

std::string Crypto::decryptAes256( const char* text,
                                   int textLength,
                                   const unsigned char* iv,
                                   const CryptoUtils::KeyAes256& key ) {
    int rc;

    EvpCipherCtxPtr ctx( EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free );
    EVP_CIPHER_CTX_init( ctx.get() );
    EVP_CIPHER_CTX_set_padding( ctx.get(), 0 );

    rc = EVP_CipherInit_ex( ctx.get(), EVP_aes_256_cbc(), nullptr, key.rawKey(), iv, 0 );  // 0 - decrypt
    if( rc != 1 )
        throw CryptoException( "EVP_CipherInit_ex failed!" );

    int cipherSize = textLength;
    auto cipherData = std::unique_ptr< unsigned char[], std::default_delete< unsigned char[] > >(
        new unsigned char[ cipherSize + 1u + CryptoUtils::KeyAes256::ms_ivSize ] );

    rc = EVP_CipherUpdate(
        ctx.get(), cipherData.get(), &cipherSize, reinterpret_cast< const unsigned char* >( text ), textLength );
    if( rc != 1 )
        throw CryptoException( "EVP_CipherUpdate failed!" );

    // Detect padding
    int cipherSizeFinal = ctx->final_used ? CryptoUtils::KeyAes256::ms_ivSize : 0;
    auto lastByte = cipherData.get() + cipherSize + cipherSizeFinal - 1;

    enum class Padding { Zero, PKCS };
    Padding padding = ( 0 < *lastByte && *lastByte <= 16 ) ? Padding::PKCS : Padding::Zero;

    // Handle padding
    switch( padding ) {
    case Padding::Zero:
        for( auto byte = lastByte; cipherSizeFinal > 0 && *byte == '\0'; cipherSizeFinal--, byte-- )
            ;
        break;
    case Padding::PKCS:
        cipherSizeFinal -= *lastByte;
        break;
    }

    std::string cipherDataStr{ cipherData.get(), cipherData.get() + cipherSize + cipherSizeFinal };
    return cipherDataStr;
}

std::string Crypto::toBase64( const std::string& data ) {
    return tiny::tools::base64::encode( data );
}

std::string Crypto::fromBase64( const std::string& data ) {
    return tiny::tools::base64::decode( data );
}

}  // namespace CryptoUtils
}  // namespace HubPlatform
