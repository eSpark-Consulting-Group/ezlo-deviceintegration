#include "cored/model/cored_item.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/visitors/value_encrypter_visitor.h"
#include "cored/model/visitors/value_decrypter_visitor.h"

#include "common_utils/cpp14.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

using namespace Resources::ItemFields;

Item::Item( const std::string& deviceId,
            const std::string& name,
            ValueType valueType,
            bool hasGetter,
            bool hasSetter,
            std::unique_ptr< Value > value,
            const std::string& suffix )
: m_deviceId( deviceId )
, m_name( name )
, m_type( std::move( valueType ) )
, m_hasGetter( hasGetter )
, m_hasSetter( hasSetter )
, m_value( std::move( value ) )
, m_show( true ) {}

const std::string& Item::getDeviceId() const {
    return m_deviceId;
}

Item::Item( Item&& ) = default;

Item::~Item() = default;

Item& Item::setLowerLimitValue( std::unique_ptr< Value > value ) {
    m_lowerLimitValue = std::move( value );
    return *this;
}

Item& Item::setUpperLimitValue( std::unique_ptr< Value > value ) {
    m_upperLimitValue = std::move( value );
    return *this;
}

Item& Item::setEnum( Enum enumeration ) {
    m_enum = std::move( enumeration );
    return *this;
}

Item& Item::setUserCodeModes( Enum userCodeModes ) {
    m_userCodeModes = std::move( userCodeModes );
    return *this;
}

Item& Item::setShow( bool show ) {
    m_show = show;
    return *this;
}

Item& Item::setElementsMaxNumber( ElementsNumber elementsMaxNumber, ContainerLevel containerLevel ) {
    m_elementsMaxNumbers[ containerLevel ] = elementsMaxNumber;
    return *this;
}

Item& Item::setWeekDayCost( WeekDayCost weekDaycost, WeekDayCost shiftedWeekDayCost ) {
    m_oneWeekDayCost = weekDaycost;
    m_oneShiftedWeekDayCost = shiftedWeekDayCost;
    return *this;
}

Item& Item::setInfo( Json info ) {
    m_info = std::move( info );
    return *this;
}

Item& Item::setUserCodeRestriction( std::string userCodeRestriction ) {
    m_userCodeRestriction = std::move( userCodeRestriction );
    return *this;
}

Item& Item::setStringRestriction( std::string stringRestriction ) {
    m_stringRestriction = std::move( stringRestriction );
    return *this;
}

Item& Item::setValueIndent( std::unique_ptr< Item::ValueIndent >&& valueIndent ) {
    m_valueIndent = std::move( valueIndent );
    return *this;
}

Json Item::toJson() const {
    auto item = JSON_EMPTY_OBJ;

    item[ NameFieldName ] = m_name;
    item[ TypeFieldName ] = m_type.toString();
    item[ ValueFieldName ] = m_value->toJson();
    if( m_lowerLimitValue ) {
        item[ ValueMinFieldName ] = m_lowerLimitValue->toJson();
    }
    if( m_upperLimitValue ) {
        item[ ValueMaxFieldName ] = m_upperLimitValue->toJson();
    }
    item[ EnumFieldName ] = m_enum;
    item[ HasGetterFieldName ] = m_hasGetter;
    item[ HasSetterFieldName ] = m_hasSetter;
    item[ DeviceIdFieldName ] = m_deviceId;
    item[ ShowFieldName ] = m_show;

    for( const auto& containerLevelToMaxNumber : m_elementsMaxNumbers ) {
        item[ getElementsMaxNumberFieldName( containerLevelToMaxNumber.first ) ] = containerLevelToMaxNumber.second;
    }

    if( !m_stringRestriction.empty() ) {
        item[ StringRestrictionFieldName ] = m_stringRestriction;
    }

    if( !m_userCodeRestriction.empty() ) {
        item[ UserCodeRestrictionFieldName ] = m_userCodeRestriction;
    }

    if( !m_userCodeModes.empty() ) {
        item[ UserCodeModesFieldName ] = m_userCodeModes;
    }

    if( !m_stringRestriction.empty() ) {
        item[ StringRestrictionFieldName ] = m_stringRestriction;
    }

    if( !m_info.empty() ) {
        item[ InfoFieldName ] = m_info;
    }
    if( m_oneWeekDayCost != 0 ) {
        item[ OneWeekDayCost ] = m_oneWeekDayCost;
    }

    if( m_oneShiftedWeekDayCost != 0 ) {
        item[ OneShiftedWeekDayCost ] = m_oneShiftedWeekDayCost;
    }

    if( m_valueIndent ) {
        auto& valueIndent = item[ ValueIndentFieldName ];
        valueIndent = JSON_EMPTY_OBJ;
        valueIndent[ ValueIndentItemFieldName ] = m_valueIndent->itemId;
        valueIndent[ ValueIndentMinItemFieldName ] = m_valueIndent->minIndent;
    }

    return item;
}

void Item::checkItemAsJson( const Json& item ) {
    auto jsonCheckEnumField = []( const Json& item ) {
        Json jsonEnum;

        if( !json_get( item, EnumFieldName, jsonEnum ) || !jsonEnum.is_array() ) {
            throw std::runtime_error{ std::string{ "fail to fetch an array at '" } + EnumFieldName + "' key" };
        }

        if( jsonEnum.size() > m_maxValueEnumSize ) {
            throw std::runtime_error{ std::string{ "object at'" } + EnumFieldName +
                                      "' key contains too much elements" };
        }

        for( const auto& val : jsonEnum ) {
            try {
                jsonCheckString( json_get_str( val ) );
            }
            catch( ... ) {
                throw std::runtime_error{ std::string{ "object at '" } + EnumFieldName +
                                          "' key: values aren't strings" };
            }
        }
    };

    /* check optional fields */
    size_t optionalFieldsCounter = 0;

    /* check mandatory fields */
    jsonCheckString( item, NameFieldName );
    jsonCheckString( item, TypeFieldName );
    jsonCheckBool( item, HasGetterFieldName );
    jsonCheckBool( item, HasSetterFieldName );
    jsonCheckString( item, DeviceIdFieldName );
    jsonCheckBool( item, ShowFieldName );
    jsonCheckEnumField( item );

    const auto valueType = ValueType::fromString( json_get_str( item, TypeFieldName ) );
    auto jsonCheckValueField = [&optionalFieldsCounter, &valueType](
                                   const Json& item, const char* key, bool isOptional ) {
        auto it = item.find( key );
        if( it == item.end() ) {
            if( isOptional ) {
                return;
            }

            throw std::runtime_error{ std::string{ "fail to fetch an object at '" } + key + "' key" };
        }

        optionalFieldsCounter += isOptional;

        ValueFactory::createItemValue( valueType, it.value() );  // throw if failed
    };

    jsonCheckValueField( item, ValueFieldName, false );
    jsonCheckValueField( item, ValueMinFieldName, true );
    jsonCheckValueField( item, ValueMaxFieldName, true );

    auto jsonCheckOptionalField = [&optionalFieldsCounter]( const Json& item, const char* key ) {
        auto it = item.find( key );
        if( it != item.end() ) {
            optionalFieldsCounter += 1;
        }
    };

    size_t containerLevel = 0;
    for( auto type = &valueType; type && type->isContainer(); type = type->getSubType().get(), ++containerLevel ) {
        jsonCheckOptionalField( item, getElementsMaxNumberFieldName( containerLevel ).c_str() );
    }

    jsonCheckOptionalField( item, StringRestrictionFieldName );
    jsonCheckOptionalField( item, UserCodeModesFieldName );
    jsonCheckOptionalField( item, UserCodeRestrictionFieldName );
    jsonCheckOptionalField( item, OneWeekDayCost );
    jsonCheckOptionalField( item, OneShiftedWeekDayCost );
    jsonCheckOptionalField( item, InfoFieldName );

    if( item.count( IdFieldName ) ) {
        jsonCheckString( item, IdFieldName );
        optionalFieldsCounter++;
    }

    {
        auto it = item.find( ValueIndentFieldName );
        if( it != item.end() ) {
            jsonCheckString( *it, ValueIndentItemFieldName );
            jsonCheckFloat( *it, ValueIndentMinItemFieldName );
            optionalFieldsCounter += 1;
        }
    }

    /* check if some 'trash' got into the json */
    if( item.size() > m_minFieldsAmount + optionalFieldsCounter ) {
        throw std::runtime_error{ "needless fields" };
    }
}

Model::ValueUPtr Item::createUserCodeValue( Json& item ) {
    const auto type = ValueType::fromString( json_get_str( item, TypeFieldName ) );
    auto foundType = &type;
    for( ; foundType && foundType->getMainType() != ValueType::Enum::UserCode;
         foundType = foundType->getSubType().get() )
        ;

    if( !foundType )
        return {};

    const auto valueIt = item.find( ValueFieldName );
    if( valueIt == item.end() )
        return {};

    return ValueFactory::createItemValue( type, valueIt.value() );
}

void Item::encryptItemAsJson( const ProvisionDataUtils::ProvisionData& provisionData, Json& item ) {
    auto valuePtr = createUserCodeValue( item );
    if( !valuePtr )
        return;

    ValueEncrypterVisitor encrypter( provisionData );
    valuePtr->accept( encrypter );
    item[ ValueFieldName ] = valuePtr->toJson();
}

void Item::decryptItemAsJson( const ProvisionDataUtils::ProvisionData& provisionData, Json& item ) {
    auto valuePtr = createUserCodeValue( item );
    if( !valuePtr )
        return;

    ValueDecrypterVisitor decrypter( provisionData );
    valuePtr->accept( decrypter );
    item[ ValueFieldName ] = valuePtr->toJson();
}

std::string Item::getElementsMaxNumberFieldName( ContainerLevel containerLevel ) {
    return containerLevel ? std::string( ElementsMaxNumberFieldName ) + std::to_string( containerLevel + 1 ) :
                            std::string( ElementsMaxNumberFieldName );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
