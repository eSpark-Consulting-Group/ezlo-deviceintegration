#pragma once

namespace HubPlatform {
namespace TwoGig {

struct ActionStatus {
    enum Enum { Started, Finished, Failed, Timeout };
};

}  // namespace TwoGig
}  // namespace HubPlatform
