#pragma once

#include <string>

namespace HubPlatform {
namespace CommonUtils {

class BackupRestoreCrypt final {
public:
    BackupRestoreCrypt( std::string version = "" );

    void setVersion( std::string version );

    bool encryptData( const std::string& data, std::string& encrypted ) const;
    bool decryptData( const std::string& data, std::string& decrypted ) const;

private:
    std::string m_version;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
