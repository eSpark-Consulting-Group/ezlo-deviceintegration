#pragma once

#include <string>

namespace HubPlatform {
namespace Opkg {

class PackageSummary final {
public:
    std::string name;
    std::string version;
};

}  // namespace Opkg
}  // namespace HubPlatform