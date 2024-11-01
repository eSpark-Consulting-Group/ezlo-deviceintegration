#pragma once

#include <cstddef>
#include <cstdint>

namespace HubPlatform {
namespace DatabaseClient {

namespace Limits {
const size_t mapSizeDefault = 10 * 1024 * 1024;
const uint64_t mapSizeMax = 1UL * 1024 * 1024 * 1024;

const size_t mapSizeMainThresholdPercent = 30;
const size_t mapSizeRamThresholdPercent = 10;
}  // namespace Limits

}  // namespace DatabaseClient
}  // namespace HubPlatform