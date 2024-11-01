#pragma once

#include <string>

namespace HubPlatform {
namespace CommonUtils {

inline std::string whatError( const std::exception& e ) {
    return "( " + std::string( typeid( e ).name() ) + " ): " + e.what();
};

}  // namespace CommonUtils
}  // namespace HubPlatform
