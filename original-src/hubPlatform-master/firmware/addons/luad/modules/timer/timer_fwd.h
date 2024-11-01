#pragma once

#include <chrono>
#include <string>
#include <memory>

namespace HubPlatform {
namespace Luad {

using TimerId = std::string;
using GroupName = std::string;
using TimerDuration = std::chrono::milliseconds;

class TimerManager;
class TimerHelper;
using TimerHelperPtr = std::shared_ptr< TimerHelper >;
using TimerHelperWPtr = std::weak_ptr< TimerHelper >;

enum class TimerType;

}  // namespace Luad

namespace Services {

class TaskLoop;
class PostponedTask;
using PostponedTaskWPtr = std::weak_ptr< PostponedTask >;

}  // namespace Services
}  // namespace HubPlatform
