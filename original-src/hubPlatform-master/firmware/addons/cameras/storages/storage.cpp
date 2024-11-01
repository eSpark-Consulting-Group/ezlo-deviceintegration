#include "cameras/storages/storage.h"

namespace HubPlatform {

void Storage::clearStorage() {}

Storage::Bytes& Storage::getStorageVolume() {
    static Bytes storageVolume( 0 );
    return storageVolume;
}

int64_t Storage::getMaxStorageVolume() {
    return std::numeric_limits< int64_t >::max();
}

ILocalStorage::Bytes ILocalStorage::MaxStorageSize( 0 );

int64_t ILocalStorage::getMaxStorageVolume() {
    return MaxStorageSize;
}

void ILocalStorage::setMaxStorageVolume( int maxStorageSizeMb ) {
    MaxStorageSize = static_cast< int64_t >( maxStorageSizeMb ) * 1024LL * 1024LL;
}

}  // namespace HubPlatform
