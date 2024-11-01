#pragma once

#include <string.h>

namespace HubPlatform {
namespace Comparators {

struct StringsIgnoreCase {
    bool operator()( const char* left, const char* right ) const {
        return strcasecmp( left, right ) < 0;
    }
};

struct StringsEqualIgnoreCase {
    bool operator()( const char* left, const char* right ) const {
        return strcasecmp( left, right ) == 0;
    }
};

}  // namespace Comparators
}  // namespace HubPlatform