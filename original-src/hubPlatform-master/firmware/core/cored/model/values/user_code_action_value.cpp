#include "cored/model/values/user_code_action_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"

#include <set>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< UserCodeActionValue > UserCodeActionValue::createValue( const Json& value ) {
    using namespace Resources::UserCodeAction;

    auto userIdIt = value.find( Fields::UserId );
    if( userIdIt == value.end() || !userIdIt->is_number_integer() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserCodeAction };
    }

    auto actionIt = value.find( Fields::Action );
    if( actionIt == value.end() || !actionIt->is_string() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserCodeAction };
    }

    static const std::set< std::string > actionSet = {
        Actions::Unknown, Actions::UserCodeEntered, Actions::InvalidUserCodeEntered
    };
    auto action = actionIt->get< std::string >();

    if( !actionSet.count( action ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserCodeAction };
    }

    return std::unique_ptr< UserCodeActionValue >{ new UserCodeActionValue{ userIdIt->get< int >(),
                                                                            std::move( action ) } };
}

std::unique_ptr< UserCodeActionValue > UserCodeActionValue::createDefaultValue() {
    using namespace Resources::UserCodeAction;
    return std::unique_ptr< UserCodeActionValue >{ new UserCodeActionValue{ InvalidUserCodeId, Actions::Unknown } };
}

UserCodeActionValue::UserCodeActionValue( int userId, std::string action )
: Value{ ValueType::Enum::UserCodeAction }
, m_userId{ userId }
, m_action{ std::move( action ) } {}

UserCodeActionValue::~UserCodeActionValue() = default;

Json UserCodeActionValue::toJson() const {
    using namespace Resources::UserLockOperation;

    return JSON( Fields::UserId << m_userId << Fields::Action << m_action );
}

void UserCodeActionValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void UserCodeActionValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool UserCodeActionValue::isEqual( const Value& other ) const {
    const auto& otherValueNameValue = valueCast< UserCodeActionValue >( other );
    return m_userId == otherValueNameValue.m_userId && m_action == otherValueNameValue.m_action;
}

void UserCodeActionValue::set( const Value& other ) {
    const auto& otherValueNameValue = valueCast< UserCodeActionValue >( other );
    m_userId = otherValueNameValue.m_userId;
    m_action = otherValueNameValue.m_action;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
