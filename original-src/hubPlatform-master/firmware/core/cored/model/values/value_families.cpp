#include "cored/model/values/value_families.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace Resources {

namespace ValueFamilies {
const char* const Numeric = "numeric";
const char* const Strings = "strings";
const char* const Array = "array";
const char* const ValuesWithLess = "valuesWithLess";
const char* const ValuesWithoutLess = "valuesWithoutLess";
}  // namespace ValueFamilies

}  // namespace Resources

std::map< std::string, std::vector< std::string > > valueTypeFamilies() {
    using namespace Model::Resources;
    std::map< std::string, std::vector< std::string > > families = {
        { ValueFamilies::Numeric, { "int", "float", "scalableValueTypes" } },
        { ValueFamilies::Strings, { "string", "token" } },
        { ValueFamilies::ValuesWithLess, { "int", "float", "scalableValueTypes", "string" } },
        // TODO: EFWR-3337: fill value types list
        { ValueFamilies::ValuesWithoutLess, { "otherTypes??" } }
    };

    return families;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform