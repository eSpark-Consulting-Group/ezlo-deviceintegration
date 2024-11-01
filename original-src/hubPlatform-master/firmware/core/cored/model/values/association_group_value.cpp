#include "cored/model/values/association_group_value.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< AssociationGroupValue > AssociationGroupValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;
    namespace Fields = Resources::AssociationGroupValueFields;

    auto configValue = createDefaultValue();

    const auto getter = makeStructGetter(
        makeMemberMapping( &AssociationGroupValue::m_name, Fields::Name ),
        makeMemberMapping( &AssociationGroupValue::m_profile, Fields::Profile, OptionalMemberPolicy() ),
        makeMemberMapping( &AssociationGroupValue::m_channel, Fields::Channel ),
        makeMemberMapping( &AssociationGroupValue::m_maxAssociationsNumber, Fields::MaxAssociationsNumber ),
        makeMemberMapping( &AssociationGroupValue::m_commands, Fields::Commands, OptionalMemberPolicy() ) );

    if( !getter( value, *configValue ) )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::AssociationGroup };

    return configValue;
}

std::unique_ptr< AssociationGroupValue > AssociationGroupValue::createDefaultValue() {
    return std::unique_ptr< AssociationGroupValue >{ new AssociationGroupValue{ ValueType::Enum::AssociationGroup } };
}

AssociationGroupValue::~AssociationGroupValue() = default;

Json AssociationGroupValue::toJson() const {
    namespace Fields = Resources::AssociationGroupValueFields;

    auto result = JSON( Fields::Name << m_name << Fields::Channel << m_channel << Fields::MaxAssociationsNumber
                                     << m_maxAssociationsNumber );

    if( !m_profile.empty() ) {
        result[ Fields::Profile ] = m_profile;
    }

    if( !m_commands.empty() ) {
        result[ Fields::Commands ] = m_commands;
    }

    return result;
}

void AssociationGroupValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void AssociationGroupValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool AssociationGroupValue::isEqual( const Value& other ) const {
    const auto& configValue = valueCast< AssociationGroupValue >( other );

    return std::tie( m_name, m_channel, m_maxAssociationsNumber, m_profile, m_commands ) ==
           std::tie( configValue.m_name,
                     configValue.m_channel,
                     configValue.m_maxAssociationsNumber,
                     configValue.m_profile,
                     configValue.m_commands );
}

void AssociationGroupValue::set( const Value& other ) {
    const auto& configValue = valueCast< AssociationGroupValue >( other );

    m_name = configValue.m_name;
    m_channel = configValue.m_channel;
    m_maxAssociationsNumber = configValue.m_maxAssociationsNumber;
    m_profile = configValue.m_profile;
    m_commands = configValue.m_commands;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
