#pragma once

#include <string>
#include <functional>

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Zwave {
namespace NvmConverterUtils {

enum class Format {
    Static6_6,
    Static6_7,
    Static6_8,

    Bridge6_6,
    Bridge6_7,
    Bridge6_8,

    Bridge7_11,
    Bridge7_12,
    Bridge7_15,
};

bool convertImage( Format inputFormat,
                   Format outputFormat,
                   const std::string& oldImagePath,
                   const std::string& newImagePath,
                   std::function< bool( Json& ) > jsonImageModifier );

bool convertVeraImageToActualFormat( const std::string& oldImagePath,
                                     const std::string& newImagePath,
                                     std::function< void( const Json& ) > jsonHandler = nullptr );

}  // namespace NvmConverterUtils
}  // namespace Zwave
}  // namespace HubPlatform
