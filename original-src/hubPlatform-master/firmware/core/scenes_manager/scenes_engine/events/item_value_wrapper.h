#pragma once

#include "cored/model/values/value.h"
#include "json_utils/json_fwd.h"
#include "variant/variant.hpp"

namespace SceneEngine {

namespace WrapperUtils {
enum class CompareOperator;
}  // namespace WrapperUtils

namespace Resources {
extern const char* Celsius;
extern const char* Fahrenheit;
};  // namespace Resources

class ItemValueWrapper {
public:
    enum class PayloadType { CoreType, UnknownType };

    ItemValueWrapper();
    ItemValueWrapper( JsonObject payload );
    ~ItemValueWrapper();
    static ItemValueWrapper fromValueAndType( JsonObject value, std::string valueType );

    ItemValueWrapper( const ItemValueWrapper& other );
    ItemValueWrapper& operator=( const ItemValueWrapper& other );
    ItemValueWrapper( ItemValueWrapper&& other );
    ItemValueWrapper& operator=( ItemValueWrapper&& other );

    const Json& getValue() const;
    const Json& getRawValue() const;

    void setPayload( JsonObject payload );
    const Json& getPayload() const;

    void setValueType( const std::string& valueType );

    bool empty() const;

    std::string getScale() const;
    std::string getValueType() const;

    bool isArray() const;
    std::vector< ItemValueWrapper > parseArrayValues() const;

    bool isValid() const noexcept;
    std::string getInvalidityReason() const noexcept;

    bool isComparableWith( const ItemValueWrapper& other ) const;

    bool operator==( const ItemValueWrapper& other ) const;
    bool operator!=( const ItemValueWrapper& other ) const;
    bool operator<( const ItemValueWrapper& other ) const;
    bool operator>( const ItemValueWrapper& other ) const;
    bool operator<=( const ItemValueWrapper& other ) const;
    bool operator>=( const ItemValueWrapper& other ) const;

private:
    using CoreValue = HubPlatform::CoreD::Model::Value;
    using CoreValueUPtr = std::unique_ptr< CoreValue >;
    using ErrorCoreValueVariant = mpark::variant< std::string, CoreValueUPtr >;

private:
    const CoreValue* getCoreValue() const noexcept;
    std::string getCoreValueParseError() const noexcept;
    void parseCoreValue();

    bool compareEqual( const ItemValueWrapper& other ) const;
    bool compareLess( const ItemValueWrapper& other ) const;
    static bool compareWrapperValues( const ItemValueWrapper& left,
                                      const ItemValueWrapper& right,
                                      WrapperUtils::CompareOperator compareOperator );

private:
    PayloadType m_payloadType = PayloadType::UnknownType;
    Json m_payload;

    ErrorCoreValueVariant m_coreValueOrError;
};

using ItemValueComparator = std::function< bool( const ItemValueWrapper&, const ItemValueWrapper& ) >;

}  // namespace SceneEngine
