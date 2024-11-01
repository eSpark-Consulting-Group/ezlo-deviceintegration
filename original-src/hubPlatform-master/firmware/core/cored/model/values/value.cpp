#include "cored/model/exceptions.h"
#include "cored/model/values/value.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

Value::Value( ValueType type )
: m_type( std::move( type ) ) {}

bool Value::operator<( const Value& other ) const {
    if( getType() != other.getType() ) {
        throw ValueError( "different types are passed to Value::operator < " );
    }

    assert( typeid( *this ) == typeid( other ) );
    return isLess( other );
}

bool Value::operator==( const Value& other ) const {
    if( getType() != other.getType() ) {
        throw ValueError( "different types are passed to Value::operator == " );
    }

    assert( typeid( *this ) == typeid( other ) );
    return isEqual( other );
}

void Value::operator=( const Value& other ) {
    if( this == &other ) {
        return;
    }
    if( getType() != other.getType() ) {
        throw ValueError( "different types are passed to Value::operator = " );
    }

    assert( typeid( *this ) == typeid( other ) );
    set( other );
}

bool Value::operator!=( const Value& other ) const {
    return !( *this == other );
}

bool Value::operator>( const Value& other ) const {
    return other < *this;
}

bool Value::isLess( const Value& other ) const {
    throw ValueError( "isLess not implemented" );
}

const ValueType& Value::getType() const {
    return m_type;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
