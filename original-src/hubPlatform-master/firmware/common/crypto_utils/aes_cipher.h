#pragma once

#include "crypto_utils/cipher.h"

namespace HubPlatform {
namespace CryptoUtils {

class AesCipher : public Cipher {
public:
    enum class Mode { ECB, CBC, CFB1, CFB8, OFB, CTR };

    AesCipher( Mode mode, std::string key );
    AesCipher( Mode mode, std::string key, std::string iv, bool enablePadding = true );

    const EVP_CIPHER* getCipher() const override;
    const std::string& getKey() const override;
    const std::string& getIV() const override;
    bool isPaddingEnabled() const override;
    std::size_t getEncryptedDataSize( std::size_t inputDataSize ) const override;

    static std::string toString( Mode type );
    static Mode toMode( std::string mode );

private:
    const EVP_CIPHER* getCipher( Mode type, std::size_t keySize ) const;

private:
    const Mode m_mode;
    const std::string m_key;
    const std::string m_iv;
    const bool m_isPaddingEnabled;
};

}  // namespace CryptoUtils
}  // namespace HubPlatform
