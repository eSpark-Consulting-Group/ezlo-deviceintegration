#include "vidoo4hub/core_manager/core_manager.h"

#include "common_utils/cpp14.h"
#include <thread>
#include <chrono>

namespace HubPlatform {
namespace Vidoo4Hub {

CoreManager::CoreManager( CoreDRemoteInterface::CoreDInterface& core )
: m_core( core ) {}

CoreDRemoteInterface::CoreDInterface& CoreManager::core() {
    return m_core;
}

void CoreManager::sync() {
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );  // Hack: wait until core finish updating database
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
