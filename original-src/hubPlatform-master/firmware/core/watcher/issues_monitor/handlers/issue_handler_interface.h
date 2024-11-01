#pragma once

namespace HubPlatform {
namespace Watcher {
namespace IssueHandlers {

class IssueHandlerInterface {
public:
    virtual ~IssueHandlerInterface() = default;
    virtual void run() = 0;
};

}  // namespace IssueHandlers
}  // namespace Watcher
}  // namespace HubPlatform
