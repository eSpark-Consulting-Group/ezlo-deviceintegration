#pragma once

#include "cored/model/values/value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ArrayValue : public Value {
public:
    using ConstValueHandler = std::function< void( const Value& ) >;
    using ValueHandler = std::function< void( Value& ) >;

    static std::unique_ptr< ArrayValue > createValue( ValueType arrayType, const Json& values );
    static std::unique_ptr< ArrayValue > createDefaultValue( ValueType arrayType );

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    void addValue( const Json& value );

    std::size_t getValuesCount() const;

    const ValueType& getValuesType() const;

    void forEachValue( const ValueHandler& handler );

    void forEachValue( const ConstValueHandler& handler ) const;

protected:
    bool isEqual( const Value& other ) const override;

    void set( const Value& other ) override;

private:
    using Value::Value;

private:
    std::vector< ValueUPtr > m_elements;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
