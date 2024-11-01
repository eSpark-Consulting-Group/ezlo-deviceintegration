#pragma once

namespace HubPlatform {
namespace Vidoo4Hub {
namespace Resources {
namespace DB {

namespace TableNames {
extern const char* const VidooCamerasDBName;
extern const char* const VidooDeviceDBName;
}  // namespace TableNames

namespace FieldNames {
extern const char* const cameraId;
extern const char* const cameraSubcategory;
extern const char* const cameraName;
extern const char* const cameraDeviceId;
extern const char* const sensorDeviceId;
extern const char* const securityThreatItemId;
extern const char* const motionItemId;
extern const char* const hotzonesItemId;
extern const char* const snapshotItemId;
extern const char* const makeRecordingItemId;
extern const char* const stopRecordingItemId;
extern const char* const rtspURL;
extern const char* const streamId;

extern const char* const vidooDeviceId;
extern const char* const objectListItemId;
extern const char* const peopleListItemId;
extern const char* const qrCodeListItemId;

extern const char* const objectListItemName;
extern const char* const peopleListItemName;
extern const char* const qrCodeListItemName;
}  // namespace FieldNames

}  // namespace DB

namespace CameraSubcategory {
extern const char* const IndoorCam;
extern const char* const OutdoorCam;
extern const char* const DoorbellCam;
}  // namespace CameraSubcategory

namespace MQTT {
extern const char* const brokerURI;
extern const char* const addonTopic;
extern const char* const vidooTopic;
}  // namespace MQTT

namespace RPC {
extern const char* const requestCameraList;
extern const char* const requestHotzoneList;
extern const char* const requestPeopleList;
extern const char* const requestObjectList;
extern const char* const requestQRCodeList;
extern const char* const updateCameraList;
extern const char* const updateObjectList;
extern const char* const updatePeopleList;
extern const char* const updateQRCodeList;
extern const char* const updateHotzoneList;
extern const char* const motion;
extern const char* const hotzoneMotion;
extern const char* const motionEnded;
extern const char* const takeSnapshot;
extern const char* const makeRecording;
extern const char* const stopRecording;
extern const char* const requestCameraStreamsList;
extern const char* const updateCameraStreamsList;
}  // namespace RPC

namespace MessageFields {
extern const char* const id;
extern const char* const objectId;
extern const char* const objectIds;
extern const char* const objectName;
extern const char* const personId;
extern const char* const peopleIds;
extern const char* const personName;
extern const char* const qrCodeId;
extern const char* const qrCodeIds;
extern const char* const qrCodeName;
extern const char* const qrCodeValue;
extern const char* const hotzoneId;
extern const char* const hotzoneName;
extern const char* const hotzonesList;
extern const char* const cameraId;
extern const char* const duration;
extern const char* const streamsList;
}  // namespace MessageFields

namespace BusMessageFields {
extern const char* const streamId;
extern const char* const localRtspUrl;
}  // namespace BusMessageFields

namespace DefaultValues {
extern const char* const defaultHotzoneName;
extern const char* const defaultHotzoneId;
constexpr int MinRecordingDuration = 15;
constexpr int MaxRecordingDuration = 60;
}  // namespace DefaultValues

}  // namespace Resources
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
