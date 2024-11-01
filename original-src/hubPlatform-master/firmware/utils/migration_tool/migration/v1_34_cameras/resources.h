#pragma once

#pragma once

namespace HubPlatform {
namespace MigrationTool {
namespace v1_34_cameras {

namespace Fields {
constexpr const char id[] = "_id";
constexpr const char category[] = "category";
constexpr const char subcategory[] = "subcategory";
constexpr const char armedCategories[] = "armedCategories";
constexpr const char subcategories[] = "subcategories";
constexpr const char type[] = "type";
constexpr const char name[] = "name";
constexpr const char hasGetter[] = "hasGetter";
constexpr const char hasSetter[] = "hasSetter";
constexpr const char show[] = "show";
constexpr const char deviceId[] = "deviceId";
constexpr const char value[] = "value";
constexpr const char enumField[] = "enum";

}  // namespace Fields

namespace Values {
constexpr const char camera[] = "camera";
constexpr const char cameraIndoor[] = "cameraIndoor";
constexpr const char makeRecording[] = "make_recording";
}  // namespace Values

namespace Tables {
constexpr const char HouseModesDb[] = "db.firmware.cored.houseModes";
constexpr const char HouseModesCamerasDb[] = "db.firmware.cored.houseModes.cameras";
constexpr const char DevicesDb[] = "db.firmware.cored.devices";
constexpr const char ItemsDb[] = "db.firmware.uid.items";
}  // namespace Tables

}  // namespace v1_34_cameras
}  // namespace MigrationTool
}  // namespace HubPlatform