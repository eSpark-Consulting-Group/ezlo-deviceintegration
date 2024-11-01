#pragma once

namespace HubPlatform {
namespace MigrationTool {
namespace v1_37_securityDevices {

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
constexpr const char itemId[] = "itemId";
constexpr const char value[] = "value";
constexpr const char enumField[] = "enum";

constexpr const char armed[] = "armed";
constexpr const char disarmedDevices[] = "disarmedDevices";
constexpr const char disarmedDefault[] = "disarmedDefault";
constexpr const char houseModeOptions[] = "houseModeOptions";
constexpr const char disarmed[] = "disarmed";
constexpr const char houseMode[] = "houseMode";

}  // namespace Fields

namespace Values {
constexpr const char securityThreat[] = "security_threat";

constexpr const char sensor[] = "sensor";
constexpr const char securitySensor[] = "security_sensor";

}  // namespace Values

namespace Tables {
constexpr const char HouseModesDb[] = "db.firmware.cored.houseModes";
constexpr const char HouseModesSecurityDevicesDb[] = "db.firmware.cored.houseModes.securityThreats";
constexpr const char DevicesDb[] = "db.firmware.cored.devices";
constexpr const char ItemsDb[] = "db.firmware.uid.items";
constexpr const char CoredSettingsDb[] = "db.firmware.cored.settings";

}  // namespace Tables

}  // namespace v1_37_securityDevices
}  // namespace MigrationTool
}  // namespace HubPlatform