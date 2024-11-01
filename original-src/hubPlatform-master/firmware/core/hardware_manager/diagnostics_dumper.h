#pragma once

#include <string>

namespace HubPlatform {
namespace HardwareManager {

class DiagnosticsDumper {
public:
    bool dump();

private:
    std::string generateDiagnosticsArchivePath() const;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
