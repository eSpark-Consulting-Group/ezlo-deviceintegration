#pragma once

#include "cored/model/values/value.h"
#include "cored/model/exceptions.h"

#include <typeinfo>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

template < typename T >
const T& valueCast( const Value& value ) {
    try {
        return dynamic_cast< const T& >( value );
    }
    catch( const std::bad_cast& e ) {
        throw ValueError( "Unable to cast value type: " + std::string( typeid( value ).name() ) + " to " +
                          std::string( typeid( T ).name() ) );
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
