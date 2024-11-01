#pragma once

#include "cored/model/values/value_type.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

/** Check for operator <()/isLess. If it throws, the value has no own implementation for isLess */
bool hasIsLess( const ValueType& valueType );
bool isValueTypeValid( const ValueType& valueType );

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
