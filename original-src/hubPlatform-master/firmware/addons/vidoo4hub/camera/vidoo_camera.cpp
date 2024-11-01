#include "vidoo4hub/camera/vidoo_camera.h"

namespace HubPlatform {
namespace Vidoo4Hub {

bool VidooCamera::operator==( const VidooCamera& camera ) const {
    if( this->cameraId != camera.cameraId )
        return false;
    if( this->cameraSubcategory != camera.cameraSubcategory )
        return false;
    if( this->cameraName != camera.cameraName )
        return false;
    if( this->cameraDeviceId != camera.cameraDeviceId )
        return false;
    if( this->sensorDeviceId != camera.sensorDeviceId )
        return false;
    if( this->securityThreatItemId != camera.securityThreatItemId )
        return false;
    if( this->motionItemId != camera.motionItemId )
        return false;
    if( this->hotzonesItemId != camera.hotzonesItemId )
        return false;
    if( this->snapshotItemId != camera.snapshotItemId )
        return false;
    if( this->makeRecordingItemId != camera.makeRecordingItemId )
        return false;
    if( this->stopRecordingItemId != camera.stopRecordingItemId )
        return false;
    if( this->rtspURL != camera.rtspURL )
        return false;
    if( this->streamId != camera.streamId )
        return false;
    return true;
}

bool VidooCamera::operator!=( const VidooCamera& camera ) const {
    return !( operator==( camera ) );
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
