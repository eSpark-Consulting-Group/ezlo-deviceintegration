#pragma once

#include <stdint.h>
#include <string>

namespace HubPlatform {
namespace Zigbee {

struct ChildData {
    uint16_t id;
    enum class Type { Unknown, Coordinator, Router, EndDevice, SleepyEndDevice } type;
    uint8_t timeout;

    std::string typeToString() const;
    std::string timeoutToString() const;
};

struct NeighborData {
    uint16_t id;
    uint8_t averageLqi;
    uint8_t age;
    uint8_t inCost;
    uint8_t outCost;
};

struct RouteData {
    uint16_t destination;
    uint16_t nextHop;
    uint8_t age;
    enum class ConcentratorType { Unknown, Low, High } concentratorType;
    enum class Status { Active, Discovering, Unknown, Unused } status;

    std::string concentratorTypeToString() const;
    std::string statusToString() const;
};

}  // namespace Zigbee
}  // namespace HubPlatform
