#pragma once
#include <chrono>
namespace HubPlatform {
namespace CoreD {
namespace WalkTest {
namespace Resources {

namespace Fields {
extern const char* const OperationId;
extern const char* const Type;
extern const char* const ProtectNotification;
}  // namespace Fields

namespace TestType {
extern const char* const Walk;
}  // namespace TestType

namespace ConfigFields {
extern const int ExtendedPeriodSeconds;
}  // namespace ConfigFields

namespace ProgressStatus {
extern const char* const Started;
extern const char* const TerminationProgress;
extern const char* const Extended;
extern const char* const Finished;
extern const char* const Failed;
}  // namespace ProgressStatus

}  // namespace Resources
}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform