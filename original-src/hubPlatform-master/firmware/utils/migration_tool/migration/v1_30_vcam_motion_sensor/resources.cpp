#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

namespace Fields {
const char* const id = "_id";
const char* const type = "type";
const char* const category = "category";
const char* const subcategory = "subcategory";
const char* const deviceTypeId = "deviceTypeId";

const char* const disarmedDefault = "disarmedDefault";
const char* const devices = "devices";
const char* const disarmedDevices = "disarmedDevices";
const char* const alarmsOffDevices = "alarmsOffDevices";

const char* const houseModesOptions = "houseModesOptions";
}  // namespace Fields

namespace Types {
const char* const sensorMotion = "sensor.motion";
const char* const securitySensor = "security_sensor";
const char* const motion = "motion";
const char* const ezvizVistaCam_MotionSensor = "ezviz_vistacam702_motion_sensor";
const char* const ezvizVistaCam_MotionSensorPrefix = "ezviz_vistacam";
const char* const ezvizVistaCam_MotionSensorSuffix = "_motion_sensor";

const char* const siren = "siren";

}  // namespace Types

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform