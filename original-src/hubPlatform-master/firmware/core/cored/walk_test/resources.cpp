#include "resources.h"

namespace HubPlatform {
namespace CoreD {
namespace WalkTest {
namespace Resources {

namespace Fields {
const char* const OperationId = "operationId";
const char* const Type = "type";
const char* const ProtectNotification = "protectNotification";
}  // namespace Fields

namespace TestType {
const char* const Walk = "walk";
}

namespace ConfigFields {
extern const int ExtendedPeriodSeconds = 5 * 60;
}  // namespace ConfigFields

namespace ProgressStatus {
const char* const Started = "started";
const char* const TerminationProgress = "termination_progress";
const char* const Extended = "extended";
const char* const Finished = "finished";
const char* const Failed = "failed";
}  // namespace ProgressStatus

}  // namespace Resources
}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform
