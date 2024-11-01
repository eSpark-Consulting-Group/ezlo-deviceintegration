#include "cameras/snapshot/snapshot_manager.h"
#include "cameras/storages/cloud_storage.h"
#include "cameras/storages/local_storage.h"

namespace HubPlatform {

SnapshotManager::SnapshotManager( std::string snapshotManagerId,
                                  ICameraInput& cameraInput,
                                  SnapshotManager::SnapshotsPath snapshotsPath )
: m_snapshoter( std::move( snapshotManagerId ) )
, m_cameraInput( cameraInput )
, m_snapshotsPath( std::move( snapshotsPath ) ) {}

void SnapshotManager::makeSnapshotForCloud( const Json& metaData,
                                            SaveFileCallback saveFile,
                                            const std::string& snapshotId,
                                            ISnapshotManager::Response responseCallback ) {
    auto storage = std::make_shared< CloudStorage >( m_snapshotsPath );
    storage->setSaveFileCallback( saveFile );
    storage->setMetaData( metaData );
    initSnapshoter( m_snapshoter );
    std::string requestId = "[cloudRequestId]: " + snapshotId;
    m_snapshoter.makeSnapshot( storage, responseCallback, requestId );
}

void SnapshotManager::makeSnapshotForUI( std::shared_ptr< Resolution > resolution,
                                         const std::string& requestId,
                                         ISnapshotManager::Response responseCallback ) {
    auto storage = std::make_shared< LocalStorage >( m_snapshotsPath );
    initSnapshoter( m_snapshoter );
    m_snapshoter.makeSnapshot( storage, responseCallback, requestId, resolution );
}

void SnapshotManager::initSnapshoter( CameraSnapshoter& cameraSnapshoter ) {
    if( !cameraSnapshoter.isInitialized() ) {
        cameraSnapshoter.init( m_cameraInput );
    }
}

}  // namespace HubPlatform
