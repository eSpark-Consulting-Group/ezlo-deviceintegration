#include "crypto_utils/digests.h"
#include "crypto_utils/crypto_exception.h"
extern "C" {
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
}
#include <iostream>

namespace HubPlatform {
namespace CryptoUtils {

Digests::Digests() {
    initAllDigests();
}

void Digests::initAllDigests() {
    OpenSSL_add_all_digests();
}

Digests& Digests::getInstance() {
    static Digests instance;
    return instance;
}

/** Calculates a digest by name.
 *  The OpenSSL_add_all_digests() should be called before the call.
 */
bool Digests::calculateDigest( const std::string& digestName, const std::string& message, std::string& digest ) {
    EVP_MD_CTX context;
    const EVP_MD* digestType;
    unsigned char digestValue[ EVP_MAX_MD_SIZE ];
    unsigned int digestSize;

    if( digestName.empty() ) {
        throw CryptoException( "Message digest name is empty: " + digestName );
    }

    digestType = EVP_get_digestbyname( digestName.c_str() );

    if( !digestType ) {
        throw CryptoException( "Unknown message digest: " + digestName );
    }

    EVP_MD_CTX_init( &context );
    EVP_DigestInit_ex( &context, digestType, nullptr );
    EVP_DigestUpdate( &context, message.data(), message.size() );
    EVP_DigestFinal_ex( &context, digestValue, &digestSize );
    EVP_MD_CTX_cleanup( &context );

    if( digestSize > 0 ) {
        digest = toHex( digestValue, digestSize );
        return true;
    }
    return false;
}

bool Digests::calculateDigest( const std::string& digestName, std::istream& stream, std::string& digest ) {
    EVP_MD_CTX context;
    const EVP_MD* digestType;
    unsigned char digestValue[ EVP_MAX_MD_SIZE ];
    unsigned int digestSize;

    if( digestName.empty() ) {
        throw CryptoException( "Message digest name is empty: " + digestName );
    }

    digestType = EVP_get_digestbyname( digestName.c_str() );

    if( !digestType ) {
        throw CryptoException( "Unknown message digest: " + digestName );
    }

    const int BLOCK_SIZE = 4096;
    char buffer[ BLOCK_SIZE ];

    EVP_MD_CTX_init( &context );
    EVP_DigestInit_ex( &context, digestType, nullptr );
    while( stream ) {
        stream.read( buffer, BLOCK_SIZE );
        EVP_DigestUpdate( &context, buffer, stream.gcount() );
    }
    EVP_DigestFinal_ex( &context, digestValue, &digestSize );
    EVP_MD_CTX_cleanup( &context );

    if( digestSize <= 0 ) {
        throw CryptoException( "Invalid digest size" );
    }

    digest = toHex( digestValue, digestSize );
    return true;
}

std::string toHex( const unsigned char* text, size_t size ) {
    std::string result;
    if( text && *text && size > 0 ) {
        result.reserve( size * 2 + 1 );

        char buf[ 4 ];
        for( size_t i = 0; i < size; i++ ) {
            snprintf( buf, sizeof( buf ), "%02x", text[ i ] );
            result += buf;
        }
    }
    return result;
}

std::string toHex( const std::string& digest ) {
    return toHex( reinterpret_cast< const unsigned char* >( digest.c_str() ), digest.size() );
}

// Digests: named functions
std::string Digests::md5( const std::string& input ) {
    MD5_CTX context;
    unsigned char digest[ MD5_DIGEST_LENGTH ];

    MD5_Init( &context );
    MD5_Update( &context, input.data(), input.size() );
    MD5_Final( digest, &context );

    return toHex( digest, MD5_DIGEST_LENGTH );
}

std::string Digests::md5( std::istream& stream ) {
    const int BLOCK_SIZE = 4 * 1024;
    MD5_CTX context;
    unsigned char digest[ MD5_DIGEST_LENGTH ];

    char buffer[ BLOCK_SIZE ];

    MD5_Init( &context );
    while( stream ) {
        stream.read( buffer, BLOCK_SIZE );
        MD5_Update( &context, buffer, stream.gcount() );
    }
    MD5_Final( digest, &context );

    return toHex( digest, MD5_DIGEST_LENGTH );
}

std::string Digests::sha1( const std::string& input ) {
    SHA_CTX context;
    unsigned char digest[ SHA_DIGEST_LENGTH ];

    SHA1_Init( &context );
    SHA1_Update( &context, input.data(), input.size() );
    SHA1_Final( digest, &context );

    return toHex( digest, SHA_DIGEST_LENGTH );
}

std::string Digests::sha1( std::istream& stream ) {
    const int BLOCK_SIZE = 4 * 1024;
    SHA_CTX context;
    unsigned char digest[ SHA_DIGEST_LENGTH ];

    char buffer[ BLOCK_SIZE ];

    SHA1_Init( &context );
    while( stream ) {
        stream.read( buffer, BLOCK_SIZE );
        SHA1_Update( &context, buffer, stream.gcount() );
    }
    SHA1_Final( digest, &context );

    return toHex( digest, SHA_DIGEST_LENGTH );
}

std::string Digests::sha256( const std::string& message ) {
    constexpr int digestLength = SHA256_DIGEST_LENGTH;
    unsigned char digest[ digestLength ];

    SHA256_CTX context;
    SHA256_Init( &context );
    SHA256_Update( &context, message.c_str(), message.length() );
    SHA256_Final( digest, &context );

    return toHex( digest, digestLength );
}

std::string Digests::sha256( std::istream& stream ) {
    const int BLOCK_SIZE = 4 * 1024;
    SHA256_CTX context;
    constexpr int digestLength = SHA256_DIGEST_LENGTH;
    unsigned char digest[ digestLength ];

    char buffer[ BLOCK_SIZE ];

    SHA256_Init( &context );
    while( stream ) {
        stream.read( buffer, BLOCK_SIZE );
        SHA256_Update( &context, buffer, stream.gcount() );
    }
    SHA256_Final( digest, &context );

    return toHex( digest, digestLength );
}

namespace {

void enumerateDigestsCallback( const OBJ_NAME* obj, void* /* arg */ ) {
    Digests::getInstance().appendDigest( obj->name );
}

}  // namespace

const Digests::DigestNames& Digests::supportedDigests() {
    if( m_supportedDigests.empty() ) {
        void* callbackArguments = nullptr;
        OBJ_NAME_do_all( OBJ_NAME_TYPE_MD_METH, &enumerateDigestsCallback, callbackArguments );
    }
    return m_supportedDigests;
}

bool Digests::appendDigest( const std::string& name ) {
    return m_supportedDigests.insert( name ).second;
}

}  // namespace CryptoUtils
}  // namespace HubPlatform
