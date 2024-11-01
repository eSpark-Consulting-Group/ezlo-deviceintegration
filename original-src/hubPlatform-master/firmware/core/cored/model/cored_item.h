#pragma once

#include "cored/model/values/value_type.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_wrapper.h"
#include "provision_data_utils/provision_data.h"

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

/* @brief cored's representation of an item */
class Item final {
public:
    using Enum = std::vector< std::string >;
    using ElementsNumber = std::size_t;
    using ContainerLevel = std::size_t;
    using WeekDayCost = size_t;
    using ElementsMaxNumbers = std::map< ContainerLevel, ElementsNumber >;

    struct ValueIndent {
        ValueIndent( std::string itemId, float minIndent )
        : itemId( std::move( itemId ) )
        , minIndent( minIndent ) {}

        std::string itemId;
        float minIndent;
    };

public:
    Item( const std::string& deviceId,
          const std::string& name,
          ValueType valueType,
          bool hasGetter,
          bool hasSetter,
          std::unique_ptr< Value > value,
          const std::string& suffix = {} );

    Item( Item&& );
    Item& operator=( Item&& ) = delete;
    Item( const Item& ) = delete;
    Item& operator=( const Item& ) = delete;
    ~Item();

    const std::string& getDeviceId() const;

    Item& setLowerLimitValue( std::unique_ptr< Value > value );
    Item& setUpperLimitValue( std::unique_ptr< Value > value );
    Item& setEnum( Enum enumeration );
    Item& setUserCodeModes( Enum userCodeModes );
    Item& setShow( bool show );
    Item& setElementsMaxNumber( ElementsNumber elementsMaxNumber, ContainerLevel containerLevel );
    Item& setWeekDayCost( WeekDayCost weekDaycost, WeekDayCost shiftedWeekDayCost );
    Item& setUserCodeRestriction( std::string userCodeRestriction );
    Item& setStringRestriction( std::string stringRestriction );
    Item& setValueIndent( std::unique_ptr< ValueIndent >&& valueIndent );
    Item& setInfo( Json info );

    Json toJson() const;
    static void checkItemAsJson( const Json& item );
    static void encryptItemAsJson( const ProvisionDataUtils::ProvisionData& provisionData, Json& item );
    static void decryptItemAsJson( const ProvisionDataUtils::ProvisionData& provisionData, Json& item );
    static std::string getElementsMaxNumberFieldName( ContainerLevel containerLevel );

private:
    static std::unique_ptr< Value > createUserCodeValue( Json& item );

private:
    const std::string m_deviceId;  // required

    const std::string m_name;  // required (it's a type of the item)
    const ValueType m_type;    // required (a type of the item's value)
    const bool m_hasGetter;    // required (whether the item provides an ability to get a value)
    const bool m_hasSetter;    // required (whether the item provides an ability to set a value)

    std::unique_ptr< Value > m_value;
    std::unique_ptr< Value > m_lowerLimitValue;  // optional
    std::unique_ptr< Value > m_upperLimitValue;  // optional
    Enum m_enum;                                 // optional
    Enum m_userCodeModes;                        // optional

    bool m_show;                              // by default true (whether to show the item (on the UI) or not)
    ElementsMaxNumbers m_elementsMaxNumbers;  // optional (max allowed elements for each container level)
    Json m_info;                              // optional
    std::string m_userCodeRestriction;        // optional
    std::string m_stringRestriction;          // optional
    WeekDayCost m_oneWeekDayCost = 0;
    WeekDayCost m_oneShiftedWeekDayCost = 0;

    std::unique_ptr< ValueIndent > m_valueIndent;  // optional

    static const size_t m_maxValueEnumSize = 64u;
    static const size_t m_minFieldsAmount = 8u;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
