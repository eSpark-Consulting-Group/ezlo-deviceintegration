#pragma once

#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"

#include <map>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DictionaryValue : public Value {
public:
    using ValueId = std::string;
    using ConstValueHandler = std::function< void( const ValueId&, const Value& ) >;
    using ValueHandler = std::function< void( const ValueId&, Value& ) >;

    static std::unique_ptr< DictionaryValue > createValue( ValueType dictionaryType, const Json& values );
    static std::unique_ptr< DictionaryValue > createDefaultValue( ValueType dictionaryType );

    ~DictionaryValue() override;

    Json toJson() const override;

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

    void addValue( const ValueId& key, const Json& value );
    void setValue( const ValueId& key, ValueUPtr value );
    const Value* getValue( const ValueId& key ) const;
    void removeValue( const ValueId& key );

    const ValueType& getValuesType() const;
    std::size_t getValuesCount() const;

    void forEachValue( ConstValueHandler handler ) const;
    void forEachValue( ValueHandler handler );

protected:
    bool isEqual( const Value& other ) const override;
    void set( const Value& other ) override;

private:
    using Value::Value;

public:
    constexpr static std::size_t m_maxElementsAmountDefault = 2048;

private:
    std::map< ValueId, ValueUPtr > m_values;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
