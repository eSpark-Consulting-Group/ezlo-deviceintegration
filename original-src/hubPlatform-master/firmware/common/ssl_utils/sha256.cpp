#include "ssl_utils/sha256.h"

#include "base_utils/utils.h"

#include <openssl/sha.h>
#include <vector>

namespace HubPlatform {
namespace SslUtils {

std::string sha256( const std::string& data ) {
    std::vector< unsigned char > hash;
    hash.resize( SHA256_DIGEST_LENGTH );

    SHA256_CTX ctx;
    SHA256_Init( &ctx );
    SHA256_Update( &ctx, data.c_str(), data.length() );
    SHA256_Final( hash.data(), &ctx );

    return getHexString( hash );
}

}  // namespace SslUtils
}  // namespace HubPlatform
