#include "resources.h"
namespace HubPlatform {
namespace CoreD {
namespace Protect {
namespace Resources {
namespace CMSSignalAlias {
const char* const Code = "code";
const char* const Text = "text";
const char* const On = "on";
const char* const Off = "off";
const char* const Persistent = "persistent";
}  // namespace CMSSignalAlias

namespace SecurityConfig {
const char* const Name = "name";
const char* const Description = "description";
const char* const Alarm = "alarm";
const char* const SoundType = "soundType";
const char* const EntryDelay = "entryDelay";
const char* const Chime = "chime";
const char* const Follower = "follower";
const char* const ArmedWhen = "armedWhen";
const char* const RecommendCategories = "recommendCategories";
const char* const Category = "category";
const char* const SubCategory = "subcategory";
const char* const Signal = "signal";
const char* const Persistent = "persistent";
}  // namespace SecurityConfig

namespace SignalFields {
const char* const Supervisory = "supervisory";
const char* const TrippedArmed = "trippedArmed";
const char* const TamperedArmed = "tamperedArmed";
const char* const TamperedDisarmed = "tamperedDisarmed";
const char* const Bypass = "bypass";
}  // namespace SignalFields

}  // namespace Resources
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform