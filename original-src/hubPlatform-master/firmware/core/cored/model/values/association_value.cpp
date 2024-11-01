#include "cored/model/values/association_value.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< AssociationValue > AssociationValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;
    namespace Fields = Resources::AssociationValueFields;

    auto configValue = createDefaultValue();

    const auto getter = makeStructGetter(
        makeMemberMapping( &AssociationValue::m_targetNode, Fields::TargetNode ),
        makeMemberMapping( &AssociationValue::m_targetChannel, Fields::TargetChannel, OptionalMemberPolicy( false ) ) );

    if( !getter( value, *configValue ) )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Association };

    return configValue;
}

std::unique_ptr< AssociationValue > AssociationValue::createDefaultValue() {
    return std::unique_ptr< AssociationValue >{ new AssociationValue{ ValueType::Enum::Association } };
}

AssociationValue::~AssociationValue() = default;

Json AssociationValue::toJson() const {
    namespace Fields = Resources::AssociationValueFields;

    auto result = JSON( Fields::TargetNode << m_targetNode );

    if( m_targetChannel != ms_invalidChannelId ) {
        result[ Fields::TargetChannel ] = m_targetChannel;
    }

    return result;
}

void AssociationValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void AssociationValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool AssociationValue::isEqual( const Value& other ) const {
    const auto& configValue = valueCast< AssociationValue >( other );

    return std::tie( m_targetNode, m_targetChannel ) ==
           std::tie( configValue.m_targetNode, configValue.m_targetChannel );
}

void AssociationValue::set( const Value& other ) {
    const auto& configValue = valueCast< AssociationValue >( other );

    m_targetNode = configValue.m_targetNode;
    m_targetChannel = configValue.m_targetChannel;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
