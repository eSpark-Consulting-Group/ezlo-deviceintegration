#pragma once

#include "crypto_utils/crypto_key_aes256.h"
#include <string>

namespace HubPlatform {
namespace ProvisionDataUtils {

std::string encryptData( const CryptoUtils::KeyAes256& key, const std::string& data );
std::string decryptData( const CryptoUtils::KeyAes256& key, const std::string& data );

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
