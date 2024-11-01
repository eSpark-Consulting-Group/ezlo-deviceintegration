#include "resources.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {
namespace Resources {

const char* const Id = "_id";
const int TimeFrameSeconds = 1800;
const int PeriodBeforeDeleteSeriesSeconds = 3600;

namespace StatisticFields {
const char* const Id = "_id";
const char* const Hits = "hits";
const char* const Added = "added";
const char* const Updated = "updated";
const char* const Deleted = "deleted";
}  // namespace StatisticFields

namespace TimeSeriesFields {
const char* const Id = "_id";
const char* const DeviceId = "deviceId";
const char* const Hits = "hits";
const char* const Begin = "begin";
const char* const Updated = "updated";
const char* const Deleted = "deleted";
}  // namespace TimeSeriesFields

namespace ConfigFields {
const char* const HitsLimit = "hitsLimit";
const char* const InactivityWindow = "inactivityWindow";
const char* const Defaults = "defaults";
}  // namespace ConfigFields

namespace Settings {
const char* const Devices = "devices";
const char* const Version = "version";
const char* const DisabledDevices = "disabledDevices";
const char* const Limits = "limits";
const char* const Stat = "stat";
const char* const DeviceId = "deviceId";
const char* const DeviceIds = "deviceIds";
const char* const HitsLimit = "hitsLimit";
const char* const InactivityWindow = "inactivityWindow";
}  // namespace Settings

namespace DBTables {
const char* const TempStatisticDB = TempFirmwareCoredSwingerShutdownDevices;
const char* const PersistentStatisticDB = FirmwareCoredSwingerShutdownDevices;
const char* const TempTimeSeriesDB = TempFirmwareCoredSwingerShutdownTimeSeries;
const char* const PersistentTimeSeriesDB = FirmwareCoredSwingerShutdownTimeSeries;
}  // namespace DBTables

}  // namespace Resources
}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
