#pragma once

namespace HubPlatform {
namespace MigrationTool {
namespace v1_33_panic {

namespace Fields {
constexpr const char* id = "_id";
constexpr const char* category = "category";
constexpr const char* subcategory = "subcategory";
constexpr const char* armedCategories = "armedCategories";
constexpr const char* subcategories = "subcategories";
constexpr const char* type = "type";
}  // namespace Fields
namespace Values {
constexpr const char* sceneController = "scene_controller";
constexpr const char* panic = "panic";
constexpr const char* controllerPanic = "controller.panic";
}  // namespace Values

namespace Tables {
constexpr const char* devices = "db.firmware.cored.devices";
constexpr const char* houseModes = "db.firmware.cored.houseModes";
}  // namespace Tables

}  // namespace v1_33_panic
}  // namespace MigrationTool
}  // namespace HubPlatform