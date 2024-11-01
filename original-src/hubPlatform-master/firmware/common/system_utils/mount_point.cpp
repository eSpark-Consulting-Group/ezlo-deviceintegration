#include "system_utils/mount_point.h"

#include <iostream>
#include <cstring>

#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace filesystem {

std::unique_ptr< MountPoint > MountPoint::mount( const std::string device, std::string mountPoint ) {
    if( !filesystem::isBlockDeviceExist( device ) )
        throw MountPointError( "No block device detected at " + device );

    if( !mountPoint.empty() && mountPoint.back() == '/' )
        mountPoint.pop_back();

    if( mountPoint.empty() )
        throw MountPointError( "Invalid mountpoint path" );

    if( mountPoint == "/mnt" )
        throw MountPointError( "Mountpoint \"/mnt\" is prohibited. Try to use mountpoint under \"/mnt\"" );

    if( system( ( "mount | grep -q '" + mountPoint + "'" ).c_str() ) == 0 )
        throw MountPointError( "Mountpoint conflicts with existing mountpoints" );

    if( !filesystem::createFolder( mountPoint ) )
        throw MountPointError( "Can not create mount point directory: " + mountPoint );

    if( system( ( "mount -o rw " + device + " " + mountPoint ).c_str() ) != 0 )
        throw MountPointError( "Can not mount device \"" + device + "\" to \"" + mountPoint + "\"" );

    return std::unique_ptr< MountPoint >( new MountPoint( std::move( mountPoint ) ) );
}

MountPoint::~MountPoint() {
    system( "sync; echo 1 > /proc/sys/vm/drop_caches" );
    system( ( "umount " + m_mountPoint ).c_str() );
    filesystem::deleteFolder( m_mountPoint );
}

MountPoint::MountPoint( std::string mountPoint )
: m_mountPoint( std::move( mountPoint ) ) {}

}  // namespace filesystem
}  // namespace HubPlatform
