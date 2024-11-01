#include "resources.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace CloudDevices {
namespace LocalNotification {
namespace Resources {

namespace Fields {
const char* const Id = "id";
const char* const Method = "method";
const char* const Api = "api";
const char* const Params = "params";
}  // namespace Fields

namespace Methods {
const char* const CloudTCDSCurrentVersionRequest = "cloud.tcds_get_current_version";
const char* const CloudTCDSCurrentVersionRequestApiVersion = "1";
}  // namespace Methods

namespace DataSegments {
namespace LocationDataSegments {
const char* const OemId = "oem_id";
const char* const LocationId = "location_uuid";
const char* const AccountId = "account_id";
const char* const Permissions = "permissions";

}  // namespace LocationDataSegments

namespace UserDataSegments {
const char* const UserUuid = "user_uuid";
const char* const UserId = "user_id";
const char* const PermissionRole = "permission_role";
const char* const Email = "email";
const char* const EmailValidated = "email_validated";
const char* const Arn = "arn";
const char* const FirstName = "first_name";
const char* const LastName = "last_name";
const char* const Username = "username";
const char* const NotificationCategories = "notification_categories";
const char* const Permissions = "permissions";

}  // namespace UserDataSegments

namespace PartnerDataSegments {
const char* const PartnerUuid = "partner_uuid";
const char* const PartnerSection = "partner";
const char* const CustomSection = "custom";
}  // namespace PartnerDataSegments

namespace SettingsDataSegments {
const char* const OemSettings = "oem_settings";
const char* const LocalNotificationsSettings = "local_notifications_settings";
}  // namespace SettingsDataSegments

namespace ProtectDataSegments {
const char* const Provider = "provider";
}  // namespace ProtectDataSegments

namespace RegisteredDevicesDataSegments {
const char* const Uuid = "uuid";
const char* const Name = "name";
const char* const Capabilities = "capabilities";
const char* const OutBoundConnectivity = "outbound_connectivity";

}  // namespace RegisteredDevicesDataSegments

namespace MeshbotUuidDataSegments {
const char* const Uuid = "uuid";
const char* const Title = "title";
const char* const Description = "description";
const char* const Id = "id";
const char* const PartnerUuid = "partner_uuid";
const char* const OemId = "oem_id";
const char* const LocalMeshbot = "local_meshbot";
const char* const Rules = "rules";

}  // namespace MeshbotUuidDataSegments

}  // namespace DataSegments

namespace DB {
namespace Table {
const char* const LocalNotificationsDB = HaLocalNotificationsDataDbName;
const char* const MeshbotsTemplatesDB = HaMeshbotsTemplatesDbName;
}  // namespace Table

namespace Fields {
const char* const Id = "_id";
const char* const RecordType = "record_type";
const char* const CurrentVersion = "current_version";
const char* const FileVersion = "file_version";
const char* const LocationData = "location_data";
const char* const UserData = "user_data";
const char* const PartnerData = "partner_data";
const char* const SettingData = "setting_data";
const char* const ProtectData = "protect_data";
const char* const RegisteredDevicesData = "registered_devices_data";
const char* const TcdsDataInfo = "tcds_data_info";
const char* const CloudDataInfo = "cloud_data_info";
const char* const MeshbotsDataInfo = "meshbots_data_info";
const char* const ActiveCloudDataInfo = "active_cloud_data_info";
const char* const ActiveMeshbotsDataInfo = "active_meshbots_data_info";
}  // namespace Fields
}  // namespace DB

}  // namespace Resources
}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
