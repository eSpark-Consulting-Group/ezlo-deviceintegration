#include "provision_data_utils/crypto_utils.h"

#include "crypto_utils/crypto_utils.h"
#include "tiny/include/tiny/tiny_utils.h"

namespace HubPlatform {
namespace ProvisionDataUtils {

std::string encryptData( const CryptoUtils::KeyAes256& key, const std::string& data ) {
    using namespace CryptoUtils;
    std::string iv = Crypto::getInstance().generateIv< CryptoUtils::KeyAes256 >();
    std::string encryptedData = Crypto::getInstance().encryptAes256(
        data.data(), data.size(), reinterpret_cast< const unsigned char* >( iv.data() ), key );
    std::string dataToWrite = tiny::tools::base64::encode( iv + encryptedData );
    return dataToWrite;
}

std::string decryptData( const CryptoUtils::KeyAes256& key, const std::string& data ) {
    using Crypto = CryptoUtils::Crypto;
    using KeyType = CryptoUtils::KeyAes256;
    static constexpr std::size_t ivSize = KeyType::ms_ivSize;

    std::string rawData, encryptedData, decryptedData;
    const unsigned char* iv;

    rawData = tiny::tools::base64::decode( data );
    if( rawData.size() < ivSize * 2 )
        return {};

    iv = reinterpret_cast< const unsigned char* >( rawData.data() );
    encryptedData = std::string( std::next( rawData.begin(), ivSize ), rawData.end() );

    decryptedData = Crypto::getInstance().decryptAes256( encryptedData.data(), encryptedData.size(), iv, key );

    return decryptedData;
}

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
