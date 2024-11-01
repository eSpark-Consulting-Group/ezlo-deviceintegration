#pragma once

#include <string>

extern "C" {
#include <openssl/ossl_typ.h>
}

namespace HubPlatform {
namespace CryptoUtils {

class Cipher {
public:
    virtual const EVP_CIPHER* getCipher() const = 0;
    virtual const std::string& getKey() const = 0;
    virtual const std::string& getIV() const = 0;
    virtual bool isPaddingEnabled() const = 0;
    virtual std::size_t getEncryptedDataSize( std::size_t inputDataSize ) const = 0;
};

}  // namespace CryptoUtils
}  // namespace HubPlatform
