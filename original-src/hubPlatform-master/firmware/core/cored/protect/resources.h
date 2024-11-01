#pragma once

namespace HubPlatform {
namespace CoreD {
namespace Protect {
namespace Resources {
namespace CMSSignalAlias {
extern const char* const Code;
extern const char* const Text;
extern const char* const On;
extern const char* const Off;
extern const char* const Persistent;
}  // namespace CMSSignalAlias

namespace SecurityConfig {
extern const char* const Name;
extern const char* const Description;
extern const char* const Alarm;
extern const char* const SoundType;
extern const char* const EntryDelay;
extern const char* const Chime;
extern const char* const Follower;
extern const char* const ArmedWhen;
extern const char* const RecommendCategories;
extern const char* const Category;
extern const char* const SubCategory;
extern const char* const Signal;
extern const char* const Persistent;
}  // namespace SecurityConfig

namespace SignalFields {
extern const char* const Supervisory;
extern const char* const TrippedArmed;
extern const char* const TamperedArmed;
extern const char* const TamperedDisarmed;
extern const char* const Bypass;
}  // namespace SignalFields
}  // namespace Resources
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform