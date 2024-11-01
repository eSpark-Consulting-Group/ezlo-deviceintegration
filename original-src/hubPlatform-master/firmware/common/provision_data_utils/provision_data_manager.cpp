#include "provision_data_utils/provision_data_manager.h"
#include "provision_data_utils/crypto_utils.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

#include <fstream>
#include <random>
#include <unistd.h>

namespace HubPlatform {
namespace ProvisionDataUtils {

namespace {

#if( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) )
static const std::string BootDevice = " /dev/mmcblk1p1 ";
#elif( defined( __HUB_TYPE_BEEWAVE__ ) )
static const std::string BootDevice = " /dev/mmcblk0p1 ";
#elif( defined( __HUB_TYPE_G150__ ) )
static const std::string BootDevice = " /dev/ubi0_0 ";
#endif

static const std::string MountPoint = "/mnt/factory";
static const std::string MasterFactoryFile = "factory.dat";
static const std::string DeltaFactoryFile = "factory2.dat";
static const std::string ControllerFile = "controller.dat";

class Mounter {
public:
    static std::unique_ptr< Mounter > mount( bool readonly ) {
        std::string flags;
        if( readonly ) {
            flags += " -o ro";
        }

#if defined( __HUB_TYPE_G150__ )
        if( !filesystem::createFolder( MountPoint ) )
            return nullptr;

        // Assuming ubiattach is already called from /etc/init.d/mount_ubi_volumes
        if( system( ( "mount -t ubifs " + flags + BootDevice + MountPoint ).c_str() ) != 0 )
            return nullptr;
#elif( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ ) )
        if( !filesystem::createFolder( MountPoint ) )
            return nullptr;

        if( system( ( "mount -t vfat " + flags + BootDevice + MountPoint ).c_str() ) != 0 )
            return nullptr;
#endif
        return std::unique_ptr< Mounter >( new Mounter() );
    }

    ~Mounter() {
        system( ( "umount " + MountPoint ).c_str() );
        filesystem::deleteFolder( MountPoint );
    }

private:
    Mounter() = default;
};

std::string readAndDecrypt( const std::string& path ) {
    auto stream = std::make_unique< std::ifstream >();
    stream->open( path );

    if( !stream->is_open() ) {
        return {};
    }

    return decryptData( *CryptoUtils::getProvisionKey(), { std::istreambuf_iterator< char >( *stream ), {} } );
}

void encryptAndSave( const std::string& data, const std::string& path ) {
    std::ofstream ofs( path, std::ofstream::trunc );
    ofs << encryptData( *CryptoUtils::getProvisionKey(), data );
    ofs.flush();
}

}  // namespace

ProvisionDataManager& getManager() {
    static ProvisionDataManager manager;
    return manager;
}

ProvisionDataManager::ProvisionDataManager() {
    checkout();
    readData();
}

const ProvisionData& ProvisionDataManager::getProvisionData() const {
    return m_merged;
}

bool ProvisionDataManager::checkout() {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    if( filesystem::isFileExist( Environment::getProvisionDataFolder() + "/" + ControllerFile ) &&
        !filesystem::isFileExist( Environment::getDataFolder() + "/" + ControllerFile ) )
        if( !filesystem::copyFile( Environment::getProvisionDataFolder() + "/" + ControllerFile,
                                   Environment::getDataFolder() + "/" + ControllerFile ) )
            return false;

    return true;
#endif

    if( filesystem::isFileExist( Environment::getDataFolder() + "/" + DeltaFactoryFile ) ) {
        filesystem::copyFile( Environment::getDataFolder() + "/" + DeltaFactoryFile,
                              Environment::getProvisionDataFolder() + "/" + DeltaFactoryFile );
    }

    if( filesystem::isFileExist( Environment::getProvisionDataFolder() + "/" + MasterFactoryFile ) &&
        filesystem::isFileExist( Environment::getDataFolder() + "/" + ControllerFile ) ) {
        return true;
    }

    auto mounter = Mounter::mount( true );
    if( !mounter )
        return false;

    if( filesystem::isFileExist( MountPoint + "/" + MasterFactoryFile ) ) {
        if( !filesystem::copyFile( MountPoint + "/" + MasterFactoryFile,
                                   Environment::getProvisionDataFolder() + "/" + MasterFactoryFile ) ) {
            return false;
        }
    }

    return true;
}

bool ProvisionDataManager::commit( bool factory, bool factory2 ) {
#if defined( __HUB_TYPE_VHUB__ )
    return true;
#endif

#if !defined( __HUB_TYPE_LOCAL__ )
    if( factory ) {
        auto mounter = Mounter::mount( false );
        if( !mounter )
            return false;

        if( !filesystem::copyFile( Environment::getProvisionDataFolder() + "/" + MasterFactoryFile,
                                   MountPoint + "/" + MasterFactoryFile ) )
            return false;
    }
#endif

    if( factory2 ) {
        if( !filesystem::copyFile( Environment::getProvisionDataFolder() + "/" + DeltaFactoryFile,
                                   Environment::getDataFolder() + "/" + DeltaFactoryFile ) )
            return false;
    }

    sync();
    return true;
}

void ProvisionDataManager::readData() {
    auto factory = readAndDecrypt( Environment::getProvisionDataFolder() + "/" + MasterFactoryFile );
    auto factory2 = readAndDecrypt( Environment::getProvisionDataFolder() + "/" + DeltaFactoryFile );
    auto controllerKey = readAndDecrypt( Environment::getDataFolder() + "/" + ControllerFile );

    if( !factory.empty() ) {
        m_factory.fillFromJson( Json::parse( factory ) );
    }

    m_merged = m_factory;
    if( !factory2.empty() ) {
        m_merged.fillFromJson( Json::parse( factory2 ) );
    }

    if( controllerKey.empty() ) {
        std::random_device rd;
        for( int i = 0; i < CryptoUtils::KeyAes256::ms_keySize / 4u; ++i ) {
            unsigned rnd = rd();
            controllerKey.append( reinterpret_cast< char* >( &rnd ), 4u );
        }
        encryptAndSave( controllerKey, Environment::getDataFolder() + "/" + ControllerFile );
        sync();
    }

    m_merged.setControllerKey( controllerKey.c_str() );
}

bool ProvisionDataManager::init( const ProvisionData& data ) {
    if( !data.isValid() )
        return false;

    if( m_factory.isValid() && data.configVersion <= m_factory.configVersion )
        return false;

    m_factory = data;

    encryptAndSave( m_factory.toJson().dump(), Environment::getProvisionDataFolder() + "/" + MasterFactoryFile );

    return commit( true, false );
}

UpdateStatus ProvisionDataManager::update( const ProvisionData& data ) {
    if( !data.isValid() )
        return UpdateStatus::Failed;

    if( m_merged.isValid() && data.configVersion <= m_merged.configVersion )
        return UpdateStatus::Failed;

    const auto updateStatus = compareProvisioning( m_merged, data );

    CryptoUtils::KeyAes256 controllerKey = *m_merged.getControllerKey();
    m_merged = data;
    m_merged.setControllerKey( std::move( controllerKey ) );

    auto baseJson = m_factory.toJson();
    auto mergedJson = m_merged.toJson();

    JsonObject diffJson;
    for( auto it = mergedJson.begin(); it != mergedJson.end(); ++it ) {
        const std::string key = it.key();
        auto value = it.value();

        auto it2 = baseJson.find( key );
        if( it2 != baseJson.end() && it2.value() == value )
            continue;

        diffJson[ key ] = value;
    }

    encryptAndSave( diffJson.dump(), Environment::getProvisionDataFolder() + "/" + DeltaFactoryFile );

    return commit( false, true ) ? updateStatus : UpdateStatus::Failed;
}

UpdateStatus ProvisionDataManager::compareProvisioning( const ProvisionData& oldData,
                                                        const ProvisionData& newData ) const {
    if( ( oldData.pkDevice != newData.pkDevice ) || ( oldData.uuid != newData.uuid ) ||
        ( oldData.zwaveRegion != newData.zwaveRegion ) ||
        ( oldData.defaultSshPassword != newData.defaultSshPassword ) ||
        ( oldData.defaultWifiSsid != newData.defaultWifiSsid ) ||
        ( oldData.defaultWifiPassword != newData.defaultWifiPassword ) ||
        ( oldData.timeServers != newData.timeServers ) ) {
        return UpdateStatus::RestartRequired;
    }

    if( ( oldData.cloudServer != newData.cloudServer ) || ( oldData.provisionServer != newData.provisionServer ) ||
        ( oldData.sslPrivateKey != newData.sslPrivateKey ) || ( oldData.sslPublicKey != newData.sslPublicKey ) ||
        ( oldData.sslSharedKey != newData.sslSharedKey ) ||
        ( oldData.signingCaCertificate != newData.signingCaCertificate ) ) {
        return UpdateStatus::ReconnectRequired;
    }

    return UpdateStatus::NothingRequired;
}

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
