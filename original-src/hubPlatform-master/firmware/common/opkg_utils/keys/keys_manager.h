#pragma once

#include "opkg_utils/keys/key_file.h"

#include <functional>
#include <vector>

namespace HubPlatform {
namespace Opkg {

class KeysManager final {
public:
    using FingerprintCallback = std::function< bool( std::string ) >;
    using Fingerprints = std::vector< std::string >;

    KeysManager( std::string keysDirectoryPath );

    bool forEachFingerprint( FingerprintCallback callback ) const;
    Fingerprints getFingerprints() const;
    KeyFile getKeyFile( const std::string& fingerprint ) const;

    bool addKeyFile( KeyFile key );
    bool removeKeyFile( std::string fingerprint );
    bool clearKeyFiles();

private:
    std::string getKeyFilePath( const std::string& fingerprint ) const;

private:
    std::string m_keysDirectoryPath;
};

}  // namespace Opkg
}  // namespace HubPlatform