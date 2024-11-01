#pragma once

#include "cameras/snapshot/snapshot_manager_interface.h"

namespace HubPlatform {

class SnapshotManager : public ISnapshotManager {
public:
    using SnapshotsPath = std::string;

    SnapshotManager( std::string snapshotManagerId, ICameraInput& cameraInput, SnapshotsPath snapshotsPath );

    void makeSnapshotForCloud( const Json& metaData,
                               SaveFileCallback saveFile,
                               const std::string& snapshotId,
                               Response responseCallback ) override;
    void makeSnapshotForUI( std::shared_ptr< Resolution > resolution,
                            const std::string& requestId,
                            Response responseCallback ) override;

private:
    void initSnapshoter( CameraSnapshoter& cameraSnapshoter );

private:
    CameraSnapshoter m_snapshoter;
    ICameraInput& m_cameraInput;
    SnapshotsPath m_snapshotsPath;
};

}  // namespace HubPlatform
