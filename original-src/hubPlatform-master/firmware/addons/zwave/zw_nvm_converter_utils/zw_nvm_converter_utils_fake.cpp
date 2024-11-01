#include "zwave/zw_nvm_converter_utils/zw_nvm_converter_utils.h"

namespace HubPlatform {
namespace Zwave {
namespace NvmConverterUtils {

bool convertImage( Format inputFormat,
                   Format outputFormat,
                   const std::string& oldImagePath,
                   const std::string& newImagePath,
                   std::function< bool( Json& ) > jsonImageModifier ) {
    return false;
}

bool convertVeraImageToActualFormat( const std::string& oldImagePath,
                                     const std::string& newImagePath,
                                     std::function< void( const Json& ) > jsonHandler ) {
    return false;
}

}  // namespace NvmConverterUtils
}  // namespace Zwave
}  // namespace HubPlatform
