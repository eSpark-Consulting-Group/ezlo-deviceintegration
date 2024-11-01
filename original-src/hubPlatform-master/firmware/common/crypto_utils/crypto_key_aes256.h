#pragma once

#include <cstddef>
#include <array>

namespace HubPlatform {
namespace CryptoUtils {

class KeyAes256 {
public:
    static constexpr std::size_t ms_keySize = 256u / 8u;
    static constexpr std::size_t ms_ivSize = 128u / 8u;

public:
    KeyAes256();
    explicit KeyAes256( const unsigned char* key );
    ~KeyAes256();

    const unsigned char* rawKey() const;

private:
    std::array< unsigned char, ms_keySize > m_key;
};

const KeyAes256* getProvisionKey();

}  // namespace CryptoUtils
}  // namespace HubPlatform
