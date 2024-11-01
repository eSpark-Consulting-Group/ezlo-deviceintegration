#pragma once

#include "json_utils/json_fwd.h"
#include "cored/model/values/value_type.h"

#include <memory>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ValueError;
class ValueVisitor;
class ValueConstVisitor;

class Value;
using ValueUPtr = std::unique_ptr< Value >;

class Value {
public:
    Value( ValueType type );
    virtual ~Value() = default;

    virtual Json toJson() const = 0;

    virtual void accept( ValueVisitor& visitor ) = 0;
    virtual void accept( ValueConstVisitor& visitor ) const = 0;

    bool operator<( const Value& other ) const;

    bool operator==( const Value& other ) const;

    bool operator!=( const Value& other ) const;

    bool operator>( const Value& other ) const;

    void operator=( const Value& other );

    const ValueType& getType() const;

protected:
    virtual bool isLess( const Value& other ) const;

    virtual bool isEqual( const Value& other ) const = 0;

    virtual void set( const Value& other ) = 0;

private:
    const ValueType m_type;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
