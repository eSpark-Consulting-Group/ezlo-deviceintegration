#include "common_utils/backup/backup_restore_crypt.h"

#include "crypto_utils/crypto_key_aes256.h"
#include "crypto_utils/crypto_utils.h"

namespace HubPlatform {
namespace CommonUtils {

namespace {
CryptoUtils::KeyAes256 operator"" _aes256( const char* s, std::size_t ) {
    return CryptoUtils::KeyAes256( reinterpret_cast< const unsigned char* >( s ) );
}

// Keys generated with
// xxd -l 32 -p /dev/random
std::map< std::string, CryptoUtils::KeyAes256 > EncryptionKeys = {
    { "0.2",
      "\x1f\x82\xd7\x68\x57\x6c\xea\x3a\x7e\x79\xc4\xb8\x97\x6a\xb3\x1d"
      "\xbd\x6e\xbb\x12\xf4\x2c\x79\x7a\x0d\x11\xf7\x3f\x80\x5e\x5f\xc2"_aes256 },

};

static const std::string Magic = "magic";

}  // namespace

BackupRestoreCrypt::BackupRestoreCrypt( std::string version )
: m_version( std::move( version ) ) {}

void BackupRestoreCrypt::setVersion( std::string version ) {
    m_version = std::move( version );
}

bool BackupRestoreCrypt::encryptData( const std::string& data, std::string& encrypted ) const {
    auto it = EncryptionKeys.find( m_version );
    if( it == EncryptionKeys.end() ) {
        encrypted = data;
        return true;
    }

    std::string dataToEncrypt = data + Magic;

    try {
        using namespace CryptoUtils;
        std::string iv = Crypto::getInstance().generateIv< CryptoUtils::KeyAes256 >();
        std::string encryptedData =
            Crypto::getInstance().encryptAes256( dataToEncrypt.data(),
                                                 dataToEncrypt.size(),
                                                 reinterpret_cast< const unsigned char* >( iv.data() ),
                                                 it->second );
        std::string dataToWrite = iv + encryptedData;
        encrypted = dataToWrite;
    }
    catch( ... ) {
        return false;
    }
    return true;
}

bool BackupRestoreCrypt::decryptData( const std::string& data, std::string& decrypted ) const {
    auto it = EncryptionKeys.find( m_version );
    if( it == EncryptionKeys.end() ) {
        decrypted = data;
        return true;
    }

    using Crypto = CryptoUtils::Crypto;
    using KeyType = CryptoUtils::KeyAes256;
    static constexpr std::size_t ivSize = KeyType::ms_ivSize;

    std::string encryptedData, decryptedData;
    const unsigned char* iv;

    if( data.size() < ivSize * 2 )
        return false;

    iv = reinterpret_cast< const unsigned char* >( data.data() );
    encryptedData = std::string( std::next( data.begin(), ivSize ), data.end() );

    try {
        decryptedData =
            Crypto::getInstance().decryptAes256( encryptedData.data(), encryptedData.size(), iv, it->second );
    }
    catch( ... ) {
        return false;
    }

    if( decryptedData.size() < Magic.size() )
        return false;

    if( decryptedData.compare( decryptedData.size() - Magic.size(), Magic.size(), Magic ) )
        return false;

    decrypted = decryptedData.substr( 0, decryptedData.size() - Magic.size() );
    return true;
}

}  // namespace CommonUtils
}  // namespace HubPlatform
