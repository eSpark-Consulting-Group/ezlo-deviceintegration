#include "cored/model/values/value_type_traits.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

bool isValueTypeValid( const ValueType& valueType ) {
    using namespace CoreD::Model;
    if( valueType == ValueType::Enum::NoValue || valueType == ValueType::Enum::SimpleTypesCount ) {
        return false;
    }
    return true;
}

bool hasIsLess( const ValueType& valueType ) {
    auto valueUPtr = ValueFactory::createDefaultItemValue( valueType );
    try {
        *valueUPtr < *valueUPtr;  // need only to catch exception from here
        return true;
    }
    catch( const CoreD::Model::ValueError& ex ) {
        return false;
    }
    catch( const CoreD::Model::UnexpectedValueType& ex ) {
        return false;
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform