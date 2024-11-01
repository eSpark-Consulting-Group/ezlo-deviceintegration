#pragma once

#include "addon_interfaces/addon.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeTaskLoop final : public Services::TaskLoop {
public:
    static ZigbeeTaskLoop& init( std::function< void() > initAction );
    static ZigbeeTaskLoop& getInstance();
    static void stopLoop();

private:
    ZigbeeTaskLoop() = delete;
    ZigbeeTaskLoop( std::function< void() > initAction );

private:
    static std::unique_ptr< ZigbeeTaskLoop > ms_taskLoop;
    static std::thread ms_zigbeeThread;
};

}  // namespace Zigbee
}  // namespace HubPlatform
