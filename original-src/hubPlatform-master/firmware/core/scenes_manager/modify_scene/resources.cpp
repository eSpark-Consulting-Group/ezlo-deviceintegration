#include "resources.h"

namespace SceneEngine {

namespace Fields {
const char* ValueField = "value";
const char* StartValueField = "startValue";
const char* EndValueField = "endValue";
const char* ScaleField = "scale";
const char* TypeField = "type";
const char* IdField = "_id";
const char* FieldsField = "fields";
const char* ItemField = "item";
const char* NameField = "name";
const char* ArrayIndex = "__array_index";

const char* BlockOptions = "blockOptions";
const char* Method = "method";
}  // namespace Fields

namespace Types {
const char* BoolType = "bool";
const char* IntType = "int";
const char* FloatType = "float";
const char* StringType = "string";
const char* TokenType = "token";
}  // namespace Types

namespace MethodNames {
const char* IsItemState = "isItemState";
const char* CompareNumbers = "compareNumbers";
const char* CompareNumberRange = "compareNumberRange";
const char* SetItemValue = "setItemValue";
const char* AndOperator = "and";
const char* NotOperator = "not";
const char* OrOperator = "or";
const char* XorOperator = "xor";
const char* FunctionOperator = "function";
}  // namespace MethodNames

}  // namespace SceneEngine
