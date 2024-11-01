
#include "common_utils/math.h"

#include <cmath>

namespace HubPlatform {
namespace CommonUtils {

bool approximatelyEqual( float first, float second, float epsilon ) {
    return ( fabs( first - second ) < epsilon );
}

}  // namespace CommonUtils
}  // namespace HubPlatform
