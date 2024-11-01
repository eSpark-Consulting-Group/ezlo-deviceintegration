#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Vidoo4Hub {
namespace Resources {
namespace DB {

namespace TableNames {
const char* const VidooCamerasDBName = "db.addons.vidoo4hub.cameras";
const char* const VidooDeviceDBName = "db.addons.vidoo4hub.vidoo_device";
}  // namespace TableNames

namespace FieldNames {
const char* const cameraId = "camera_id";
const char* const cameraSubcategory = "camera_subcategory";
const char* const cameraName = "camera_name";
const char* const cameraDeviceId = "device_id";
const char* const sensorDeviceId = "sensor_id";
const char* const securityThreatItemId = "security_threat_item_id";
const char* const motionItemId = "motion_item_id";
const char* const hotzonesItemId = "hotzones_item_id";
const char* const snapshotItemId = "snapshot_item_id";
const char* const makeRecordingItemId = "make_recording_item_id";
const char* const stopRecordingItemId = "stop_recording_item_id";
const char* const rtspURL = "rtsp_url";
const char* const streamId = "stream_id";

const char* const vidooDeviceId = "vidoo_device_id";
const char* const objectListItemId = "object_list_item_id";
const char* const peopleListItemId = "people_list_item_id";
const char* const qrCodeListItemId = "qrcode_list_item_id";

const char* const objectListItemName = "detectable_objects";
const char* const peopleListItemName = "detectable_persons";
const char* const qrCodeListItemName = "detectable_qrcodes";
}  // namespace FieldNames

}  // namespace DB

namespace CameraSubcategory {
const char* const IndoorCam = "indoor_cam";
const char* const OutdoorCam = "outdoor_cam";
const char* const DoorbellCam = "doorbell_cam";
}  // namespace CameraSubcategory

namespace MQTT {
const char* const brokerURI = "tcp://127.0.0.1:51883";
const char* const addonTopic = "vidoo4hub";
const char* const vidooTopic = "fwm_2_ezlo";
}  // namespace MQTT

namespace RPC {
const char* const requestCameraList = "request_camera_list";
const char* const requestHotzoneList = "request_hotzone_list";
const char* const requestPeopleList = "request_people_list";
const char* const requestObjectList = "request_object_list";
const char* const requestQRCodeList = "request_qrcode_list";
const char* const updateCameraList = "update_camera_list";
const char* const updateObjectList = "update_object_list";
const char* const updatePeopleList = "update_people_list";
const char* const updateQRCodeList = "update_qrcode_list";
const char* const updateHotzoneList = "update_hotzone_list";
const char* const motion = "motion";
const char* const hotzoneMotion = "hotzone_motion";
const char* const takeSnapshot = "take_snapshot";
const char* const makeRecording = "make_recording";
const char* const stopRecording = "stop_recording";
const char* const requestCameraStreamsList = "request_camera_streams_list";
const char* const updateCameraStreamsList = "update_camera_streams_list";
}  // namespace RPC

namespace MessageFields {
const char* const id = "id";
const char* const objectId = "object_id";
const char* const objectIds = "object_ids";
const char* const objectName = "object_name";
const char* const personId = "person_id";
const char* const peopleIds = "people_ids";
const char* const personName = "person_name";
const char* const qrCodeId = "qrcode_id";
const char* const qrCodeIds = "qrcode_ids";
const char* const qrCodeName = "qrcode_name";
const char* const qrCodeValue = "qrcode_value";
const char* const hotzoneId = "hotzone_id";
const char* const hotzoneName = "hotzone_name";
const char* const hotzonesList = "hotzones";
const char* const cameraId = "camera_id";
const char* const duration = "duration";
const char* const streamsList = "streams";
}  // namespace MessageFields

namespace BusMessageFields {
const char* const streamId = "streamId";
const char* const localRtspUrl = "localRtspUrl";
}  // namespace BusMessageFields

namespace DefaultValues {
const char* const defaultHotzoneName = "Default";
const char* const defaultHotzoneId = "";
}  // namespace DefaultValues

}  // namespace Resources
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
