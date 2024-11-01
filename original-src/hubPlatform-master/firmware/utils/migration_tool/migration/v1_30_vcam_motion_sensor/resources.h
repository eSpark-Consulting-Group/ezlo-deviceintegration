#pragma once

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

namespace Fields {
extern const char* const id;
extern const char* const type;
extern const char* const category;
extern const char* const subcategory;
extern const char* const deviceTypeId;

extern const char* const disarmedDefault;
extern const char* const devices;
extern const char* const disarmedDevices;
extern const char* const alarmsOffDevices;

extern const char* const houseModesOptions;
}  // namespace Fields

namespace Types {
extern const char* const sensorMotion;
extern const char* const securitySensor;
extern const char* const motion;
extern const char* const ezvizVistaCam_MotionSensor;
extern const char* const ezvizVistaCam_MotionSensorPrefix;
extern const char* const ezvizVistaCam_MotionSensorSuffix;

extern const char* const siren;
}  // namespace Types

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform