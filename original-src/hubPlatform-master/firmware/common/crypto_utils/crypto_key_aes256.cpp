#include "crypto_utils/crypto_key_aes256.h"

#include <cstring>

namespace HubPlatform {
namespace CryptoUtils {

KeyAes256::KeyAes256() {
    m_key.fill( 0 );
}

KeyAes256::KeyAes256( const unsigned char* key ) {
    std::memcpy( m_key.data(), key, ms_keySize * sizeof( unsigned char ) );
}

KeyAes256::~KeyAes256() {
    m_key.fill( 0 );
}

const unsigned char* KeyAes256::rawKey() const {
    return m_key.data();
}

const KeyAes256* getProvisionKey() {
#if defined( __HUB_TYPE_VHUB__ )
    // hex: 3F 8D E1 B4 F1 0D BE 27 DE B7 F9 59 2B 95 E1 5D D8 D2 36 FB B4 4F A0 AF 63 B4 FC 08 F2 23 12 36
    // base64: P43htPENvifet/lZK5XhXdjSNvu0T6CvY7T8CPIjEjY=
    static const unsigned char provisionKeyBytes[ 32 ] = { 0x3F, 0x8D, 0xE1, 0xB4, 0xF1, 0x0D, 0xBE, 0x27,
                                                           0xDE, 0xB7, 0xF9, 0x59, 0x2B, 0x95, 0xE1, 0x5D,
                                                           0xD8, 0xD2, 0x36, 0xFB, 0xB4, 0x4F, 0xA0, 0xAF,
                                                           0x63, 0xB4, 0xFC, 0x08, 0xF2, 0x23, 0x12, 0x36 };
#else
    // hex: 04 FB FA AD 1B 6F FC B1 19 63 F8 66 9B F9 DD E0 BF F0 D1 8E 8F 8A 46 9F 5B A4 F1 EE 87 0F F9 7B
    // base64: BPv6rRtv/LEZY/hmm/nd4L/w0Y6PikafW6Tx7ocP+Xs=
    static const unsigned char provisionKeyBytes[ 32 ] = { 0x04, 0xFB, 0xFA, 0xAD, 0x1B, 0x6F, 0xFC, 0xB1,
                                                           0x19, 0x63, 0xF8, 0x66, 0x9B, 0xF9, 0xDD, 0xE0,
                                                           0xBF, 0xF0, 0xD1, 0x8E, 0x8F, 0x8A, 0x46, 0x9F,
                                                           0x5B, 0xA4, 0xF1, 0xEE, 0x87, 0x0F, 0xF9, 0x7B };
#endif
    static const KeyAes256 provisionKey( provisionKeyBytes );

    return &provisionKey;
}

}  // namespace CryptoUtils
}  // namespace HubPlatform
