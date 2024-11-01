#pragma once

#include <map>
#include <vector>

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace Resources {

namespace ValueFamilies {
extern const char* const Numeric;
extern const char* const Strings;
extern const char* const Array;
extern const char* const ValuesWithLess;
extern const char* const ValuesWithoutLess;
}  // namespace ValueFamilies

}  // namespace Resources

std::map< std::string, std::vector< std::string > > valueTypeFamilies();

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
