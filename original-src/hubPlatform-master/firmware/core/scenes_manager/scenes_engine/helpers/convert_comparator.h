#pragma once

#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"
#include "jsonrpc/error_codes.h"
#include "scenes_manager/scenes_engine/exceptions.h"

namespace SceneEngine {

class ConvertComparatorException : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

ItemValueComparator convertComparator( const std::string& comparator, bool valueTypeHasIsLess );
}  // namespace SceneEngine