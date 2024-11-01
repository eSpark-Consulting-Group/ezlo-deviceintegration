#include "migration_tool/provisioning.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "base_utils/return_action.h"
#include "provision_data_utils/provision_data_manager.h"
#include "jsonrpc/jsonrpc.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Provisioning {

const char* cloudCertificateFile = "/cloud.ca";
const char* hubSslKeyPrivateFile = "/cloud.key";
const char* hubSslKeyPublicFile = "/cloud.crt";
const char* zwaveConfigFile = "/zwave.cfg";
const char* shadowFile = "/etc/shadow";
const char* OpkgTrustedKeyFile = "/opkg.key";

Provisioning::Provisioning()
: m_provisionData( ProvisionDataUtils::getManager().getProvisionData() ) {}

bool Provisioning::hasProvisionInformation() {
    return m_provisionData.isValid();
}

bool Provisioning::setCloudCertificate() {
    LOGGER_INFO( "-----------------" );
    return writeConfigDataToFile( m_provisionData.signingCaCertificate,
                                  Environment::getCertificateFolder() + cloudCertificateFile );
}

bool Provisioning::setHubSslKeyPrivate() {
    LOGGER_INFO( "-----------------" );
    return writeConfigDataToFile( m_provisionData.sslPrivateKey,
                                  Environment::getCertificateFolder() + hubSslKeyPrivateFile );
}

bool Provisioning::setHubSslKeyPublic() {
    LOGGER_INFO( "-----------------" );
    return writeConfigDataToFile( m_provisionData.sslSharedKey,
                                  Environment::getCertificateFolder() + hubSslKeyPublicFile );
}

bool Provisioning::setRootPassword() {
    LOGGER_INFO( "-----------------" );

#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    LOGGER_INFO( "Running on local machine - skip root pass update" );
    return true;
#endif

    const std::string shadowFileCopy = std::string( shadowFile ) + "_copy";
    const std::string commandCopyShadowFile = std::string( "cp " ) + shadowFile + " " + shadowFileCopy;
    if( std::system( commandCopyShadowFile.c_str() ) != 0 ) {
        LOGGER_WARN( "backup shadow file fail" );
        return false;
    }
    ReturnAction removeShadowFileCopy( [shadowFileCopy]() {
        std::string command = "rm -f " + shadowFileCopy;
        std::system( command.c_str() );
    } );
    if( m_provisionData.defaultSshPassword.empty() ) {
        LOGGER_WARN( "No default ssh password found" );
        return false;
    }
    if( !replaceRootPassword( shadowFileCopy, m_provisionData.defaultSshPassword ) ) {
        LOGGER_WARN( "replace root password fail" );
        return false;
    }
    const std::string commandReplaceShadowFile = "cp " + shadowFileCopy + " " + shadowFile;
    if( std::system( commandReplaceShadowFile.c_str() ) != 0 ) {
        LOGGER_WARN( "replace shadow file fail" );
        return false;
    }
    return true;
}

bool Provisioning::setZwaveRegion() {
    LOGGER_INFO( "-----------------" );
    std::unique_ptr< std::ofstream > file =
        createFileAndCheckValue( Environment::getConfigsFolder() + zwaveConfigFile, m_provisionData.zwaveRegion );
    if( !file ) {
        return false;
    }
    JsonObject zwaveRegion = JSON_EMPTY_OBJ;
    json_add_str( zwaveRegion, "region", m_provisionData.zwaveRegion );
    *file << zwaveRegion.dump();
    LOGGER_INFO( "zwave data has written to file" );
    return true;
}

bool Provisioning::setOpkgTrustedKey() {
    //    LOGGER_INFO( "-----------------" );
    //    const std::string opkgTrustedKeyPath = Environment::getConfigsFolder() + OpkgTrustedKeyFile;
    //    if( !writeConfigDataToFile( m_provisionData.opkgTrustedKey, opkgTrustedKeyPath ) ) {
    //        return false;
    //    }
    //    ReturnAction removeTrustedKey( [opkgTrustedKeyPath]() { filesystem::removeFile( opkgTrustedKeyPath.c_str() );
    //    } ); const std::string command = "opkg-key add " + opkgTrustedKeyPath; if( std::system( command.c_str() ) != 0
    //    ) {
    //        LOGGER_WARN( "opkg-key add fail" );
    //        return false;
    //    }
    return true;
}

bool Provisioning::writeConfigDataToFile( const std::string& value, const std::string& filePath ) {
    std::unique_ptr< std::ofstream > file = createFileAndCheckValue( filePath, value );
    if( !file ) {
        return false;
    }
    *file << value;
    LOGGER_INFO( "data has written to file: " + filePath );
    return true;
}

std::unique_ptr< std::ofstream > Provisioning::createFileAndCheckValue( const std::string& filePath,
                                                                        const std::string& value ) {
    std::unique_ptr< std::ofstream > file = createFile( filePath );
    if( !file ) {
        LOGGER_WARN( "file isn't created: " + filePath );
        return nullptr;
    }
    if( value.empty() ) {
        LOGGER_WARN( "value is empty" );
        return nullptr;
    }
    LOGGER_INFO( "file is created: " + filePath );
    return std::move( file );
}

std::unique_ptr< std::ofstream > Provisioning::createFile( const std::string& filePath ) {
    auto file = std::make_unique< std::ofstream >( filePath );
    if( !file->good() ) {
        return nullptr;
    }
    return std::move( file );
}

bool Provisioning::replaceRootPassword( const std::string& filePath, const std::string& replaceWith ) {
    std::string fileData;
    {
        std::ifstream infile( filePath );
        if( !infile.good() ) {
            LOGGER_WARN( "file isn't opened: " + filePath );
            return false;
        }
        std::stringstream ss;
        ss << infile.rdbuf();
        fileData = std::move( ss.str() );
    }
    const std::string beginReplace = "root:";
    const std::string endReplace = ":";
    auto rootPos = fileData.find( beginReplace );
    if( rootPos == std::string::npos ) {
        LOGGER_WARN( "begin position of replacement field isn't found" );
        return false;
    }
    auto beginPos = rootPos + beginReplace.size();
    auto endPos = fileData.find( endReplace, beginPos );
    if( endPos == std::string::npos ) {
        LOGGER_WARN( "end position of replacement field isn't found" );
        return false;
    }
    fileData.replace( beginPos, endPos - beginPos, replaceWith );
    std::unique_ptr< std::ofstream > outfile = createFile( filePath );
    if( !outfile ) {
        LOGGER_WARN( "file isn't created: " + filePath );
        return false;
    }
    *outfile << fileData;
    LOGGER_INFO( "ok" );
    return true;
}

}  // namespace Provisioning
}  // namespace HubPlatform
