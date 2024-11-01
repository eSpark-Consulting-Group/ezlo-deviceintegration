#pragma once

#include <string>

namespace HubPlatform {
namespace Metrics {
namespace UT {
// These are dummy certificates and SHOULD NOT be used anywhere except unit tests!
extern std::string clientCrt;

extern std::string clientPrivateKey;

extern std::string serverPublicKey;

extern char const* jsonData;

}  // namespace UT
}  // namespace Metrics
}  // namespace HubPlatform
