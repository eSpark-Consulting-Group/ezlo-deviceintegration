#pragma once

namespace HubPlatform {
namespace Environment {
namespace Zigbee {

void bootInit();
void bootAssert();
void bootDeAssert();
void resetInit();
void resetAssert();
void resetDeAssert();
void resetCycle();

}  // namespace Zigbee
}  // namespace Environment
}  // namespace HubPlatform