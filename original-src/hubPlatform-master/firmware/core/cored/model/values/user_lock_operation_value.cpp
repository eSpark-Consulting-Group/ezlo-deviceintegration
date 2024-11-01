#include "cored/model/values/user_lock_operation_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"

#include <unordered_set>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< UserLockOperationValue > UserLockOperationValue::createValue( const Json& value ) {
    using namespace Resources::UserLockOperation;

    auto userIdIt = value.find( Fields::UserId );
    if( userIdIt == value.end() || !userIdIt->is_number_integer() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserLockOperation };
    }

    auto actionIt = value.find( Fields::Action );
    if( actionIt == value.end() || !actionIt->is_string() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserLockOperation };
    }

    static const std::unordered_set< std::string > actionSet = {
        Actions::Unknown, Actions::Lock, Actions::Unlock, Actions::ValidUserOutsideOfSchedule
    };
    auto action = actionIt->get< std::string >();

    if( !actionSet.count( action ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserLockOperation };
    }

    return std::unique_ptr< UserLockOperationValue >{ new UserLockOperationValue{ userIdIt->get< int >(),
                                                                                  std::move( action ) } };
}

std::unique_ptr< UserLockOperationValue > UserLockOperationValue::createDefaultValue() {
    using namespace Resources::UserLockOperation;
    return std::unique_ptr< UserLockOperationValue >{ new UserLockOperationValue{ InvalidUserCodeId,
                                                                                  Actions::Unknown } };
}

UserLockOperationValue::UserLockOperationValue( int userId, std::string action )
: Value{ ValueType::Enum::UserLockOperation }
, m_userId{ userId }
, m_action{ std::move( action ) } {}

UserLockOperationValue::~UserLockOperationValue() = default;

Json UserLockOperationValue::toJson() const {
    using namespace Resources::UserLockOperation;

    return JSON( Fields::UserId << m_userId << Fields::Action << m_action );
}

void UserLockOperationValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void UserLockOperationValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool UserLockOperationValue::isEqual( const Value& other ) const {
    const auto& otherValueNameValue = valueCast< UserLockOperationValue >( other );
    return m_userId == otherValueNameValue.m_userId && m_action == otherValueNameValue.m_action;
}

void UserLockOperationValue::set( const Value& other ) {
    const auto& otherValueNameValue = valueCast< UserLockOperationValue >( other );
    m_userId = otherValueNameValue.m_userId;
    m_action = otherValueNameValue.m_action;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
