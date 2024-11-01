#pragma once
#include <memory>

namespace HubPlatform {
namespace Services {

class Strand;
using StrandPtr = std::shared_ptr< Strand >;
using StrandId = uint64_t;

}  // namespace Services
}  // namespace HubPlatform
