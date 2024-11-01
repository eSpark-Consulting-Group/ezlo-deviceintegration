#pragma once

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {
namespace Resources {

extern const char* const Id;
extern const int TimeFrameSeconds;
extern const int PeriodBeforeDeleteSeriesSeconds;

namespace StatisticFields {
extern const char* const Id;
extern const char* const Hits;
extern const char* const Added;
extern const char* const Updated;
extern const char* const Deleted;
}  // namespace StatisticFields

namespace TimeSeriesFields {
extern const char* const Id;
extern const char* const DeviceId;
extern const char* const Hits;
extern const char* const Begin;
extern const char* const Updated;
extern const char* const Deleted;
}  // namespace TimeSeriesFields

namespace ConfigFields {
extern const char* const HitsLimit;
extern const char* const InactivityWindow;
extern const char* const Defaults;
}  // namespace ConfigFields

namespace Settings {
extern const char* const Devices;
extern const char* const Version;
extern const char* const DisabledDevices;
extern const char* const Limits;
extern const char* const Version;
extern const char* const Stat;
extern const char* const DeviceId;
extern const char* const DeviceIds;
extern const char* const HitsLimit;
extern const char* const InactivityWindow;
}  // namespace Settings

namespace DBTables {
extern const char* const TempStatisticDB;
extern const char* const PersistentStatisticDB;
extern const char* const TempTimeSeriesDB;
extern const char* const PersistentTimeSeriesDB;
}  // namespace DBTables

}  // namespace Resources
}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform