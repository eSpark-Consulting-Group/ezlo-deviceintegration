#pragma once

#include "json_utils/json_fwd.h"

#include <memory>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ValueType;

class Value;
using ValueUPtr = std::unique_ptr< Value >;

class DictionaryValue;
using DictionaryValueUPtr = std::unique_ptr< DictionaryValue >;

namespace ValueFactory {

ValueUPtr createItemValue( ValueType valueType, const Json& value );
ValueUPtr createDefaultItemValue( ValueType valueType );

}  // namespace ValueFactory
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
