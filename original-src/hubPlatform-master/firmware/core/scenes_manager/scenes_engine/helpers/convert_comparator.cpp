#include "scenes_manager/scenes_engine/helpers/convert_comparator.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

/** @brief Converts a string name of a comparator to a specific comparison function for an ItemValueWrapper class.
 *
 * For the value types that derived from CoreD::Model::Value should return available operations.
 * For classes with overridden isLess() member should return valid comparison functions for all 6 arithmetic
 *      comparisons: ==, !=, <, > , <=, >=.
 * For classes without a custom  implementation of isLess(), should return only ==, !=, and fail with exception in case
 * of other comparison methods selected. \n\n
 *
 * Throws: ConvertComparatorException
 * */
ItemValueComparator convertComparator( const std::string& comparator, bool valueTypeHasIsLess ) {
    using namespace SceneEngine::Model::Resources;
    ItemValueComparator compareFunction;

    if( comparator == ValueComparators::Equal ) {
        return std::equal_to< ItemValueWrapper >();
    }
    else if( comparator == ValueComparators::NotEqual ) {
        return std::not_equal_to< ItemValueWrapper >();
    }
    else if( valueTypeHasIsLess ) {
        if( comparator == ValueComparators::Greater ) {
            compareFunction = std::greater< ItemValueWrapper >();
        }
        else if( comparator == ValueComparators::Less ) {
            compareFunction = std::less< ItemValueWrapper >();
        }
        else if( comparator == ValueComparators::GreaterEqual ) {
            compareFunction = std::greater_equal< ItemValueWrapper >();
        }
        else if( comparator == ValueComparators::LessEqual ) {
            compareFunction = std::less_equal< ItemValueWrapper >();
        }
        else {
            throw ConvertComparatorException( "Unknown comparator passed: " + comparator );
        }
    }
    else {
        throw ConvertComparatorException( "Value type doesn't support this comparator: " + comparator );
    }
    return compareFunction;
}

}  // namespace SceneEngine