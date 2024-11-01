#pragma once

#include <vector>
#include <znet/optional.hpp>
#include <string>

namespace HubPlatform {
namespace Zwave {
namespace AntiTheftHelper {

using Bytes = std::vector< uint8_t >;
using BytesOpt = znet::utils::optional< Bytes >;

Bytes generateHint();

BytesOpt getMagicCode( const std::vector< uint8_t >& hint, const std::string& userSpecificString );

BytesOpt getMagicCode( const std::vector< uint8_t >& hint );

}  // namespace AntiTheftHelper
}  // namespace Zwave
}  // namespace HubPlatform
