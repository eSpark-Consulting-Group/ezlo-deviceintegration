#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Hub {

class Setting final {
public:
    enum class ValueType { Bool, Token, String, Int };
    using PossibleValues = std::vector< std::string >;

    explicit Setting( Json json );

    bool isValid() const;

    std::string getName() const;
    ValueType getValueType() const;

    std::string getTokenValue() const;
    std::string getStringValue() const;
    bool getBoolValue() const;
    int getIntValue() const;
    Json getValue() const;

    PossibleValues getPossibleTokenValues() const;

    bool setBoolValue( bool value );
    bool setTokenValue( const std::string& value );
    bool setStringValue( const std::string& value );
    bool setIntValue( int value );
    bool setValue( const Json& value );

    const Json& asJson() const;

    static ValueType valueTypeFromString( const std::string& valueType );

private:
    Json m_json;
};

}  // namespace Hub
}  // namespace HubPlatform
