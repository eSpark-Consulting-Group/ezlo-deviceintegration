#pragma once

namespace HubPlatform {
namespace MigrationTool {

class SystemFix {
public:
    virtual ~SystemFix() = default;

    virtual const char* getName() const = 0;
    virtual bool shouldApply() const = 0;
    virtual bool apply() = 0;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
