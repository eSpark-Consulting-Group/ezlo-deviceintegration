#include "cameras/events/stream_snapshot_events.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {

StreamSnapshotted::StreamSnapshotted( const std::string& streamId,
                                      const std::string& snapshotId,
                                      const Json& cloudStorageKey )
: CameraBaseEvent(
      "stream_snapshotted",
      JSON( "streamId" << streamId << "snapshotId" << snapshotId << "cloudStorageKey" << cloudStorageKey ) ) {}

StreamSnapshotError::StreamSnapshotError( const std::string& streamId,
                                          const std::string& snapshotId,
                                          const std::string& errorMessage )
: CameraBaseEvent( "stream_snapshot_error",
                   JSON( "streamId" << streamId << "snapshotId" << snapshotId << "errorMessage" << errorMessage ) ) {}

StreamSnapshotError::StreamSnapshotError( const std::string& streamId, const std::string& errorMessage )
: CameraBaseEvent( "stream_snapshot_error", JSON( "streamId" << streamId << "errorMessage" << errorMessage ) ) {}

StreamSnapshotError::StreamSnapshotError( const std::string& errorMessage )
: CameraBaseEvent( "stream_snapshot_error", JSON( "errorMessage" << errorMessage ) ) {}

}  // namespace HubPlatform
