#pragma once

namespace HubPlatform {
namespace CloudDevices {
namespace LocalNotification {
namespace Resources {

namespace Fields {
extern const char* const Id;
extern const char* const Method;
extern const char* const Api;
extern const char* const Params;
}  // namespace Fields

namespace Methods {
extern const char* const CloudTCDSCurrentVersionRequest;
extern const char* const CloudTCDSCurrentVersionRequestApiVersion;
}  // namespace Methods

namespace DataSegments {
namespace LocationDataSegments {
extern const char* const OemId;
extern const char* const LocationId;
extern const char* const AccountId;
extern const char* const Permissions;

}  // namespace LocationDataSegments

namespace UserDataSegments {
extern const char* const UserUuid;
extern const char* const UserId;
extern const char* const PermissionRole;
extern const char* const Email;
extern const char* const EmailValidated;
extern const char* const Arn;
extern const char* const FirstName;
extern const char* const LastName;
extern const char* const Username;
extern const char* const NotificationCategories;
extern const char* const Permissions;

}  // namespace UserDataSegments

namespace PartnerDataSegments {
extern const char* const PartnerUuid;
extern const char* const PartnerSection;
extern const char* const CustomSection;
}  // namespace PartnerDataSegments

namespace SettingsDataSegments {
extern const char* const OemSettings;
extern const char* const LocalNotificationsSettings;
}  // namespace SettingsDataSegments

namespace ProtectDataSegments {
extern const char* const Provider;
}  // namespace ProtectDataSegments

namespace RegisteredDevicesDataSegments {
extern const char* const Uuid;
extern const char* const Name;
extern const char* const Capabilities;
extern const char* const OutBoundConnectivity;

}  // namespace RegisteredDevicesDataSegments

namespace MeshbotUuidDataSegments {
extern const char* const Uuid;
extern const char* const Title;
extern const char* const Description;
extern const char* const Id;
extern const char* const PartnerUuid;
extern const char* const OemId;
extern const char* const LocalMeshbot;
extern const char* const Rules;

}  // namespace MeshbotUuidDataSegments

}  // namespace DataSegments

namespace DB {
namespace Table {
extern const char* const LocalNotificationsDB;
extern const char* const MeshbotsTemplatesDB;
}  // namespace Table

namespace Fields {
extern const char* const Id;
extern const char* const RecordType;
extern const char* const CurrentVersion;
extern const char* const FileVersion;
extern const char* const LocationData;
extern const char* const UserData;
extern const char* const PartnerData;
extern const char* const SettingData;
extern const char* const ProtectData;
extern const char* const RegisteredDevicesData;
extern const char* const TcdsDataInfo;
extern const char* const CloudDataInfo;
extern const char* const MeshbotsDataInfo;
extern const char* const ActiveCloudDataInfo;
extern const char* const ActiveMeshbotsDataInfo;
}  // namespace Fields
}  // namespace DB
}  // namespace Resources
}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
