#include "cored/model/values/usercode_value.h"

#include "cored/model/exceptions.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< UserCodeValue > UserCodeValue::createValue( const Json& value ) {
    using namespace Resources::UsercodeValueFields;
    using namespace JsonCustomGetters;

    auto result = createDefaultValue();

    const auto getter =
        makeStructGetter( makeMemberMapping( &UserCodeValue::m_userName, UserNameFieldName ),
                          makeMemberMapping( &UserCodeValue::m_userCode, UserCodeFieldName ),
                          makeMemberMapping( &UserCodeValue::m_userMode, UserModeFieldName, OptionalMemberPolicy() ) );

    if( !getter( value, *result ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::UserCode };
    }

    return result;
}

std::unique_ptr< UserCodeValue > UserCodeValue::createDefaultValue() {
    return std::unique_ptr< UserCodeValue >{ new UserCodeValue{ {}, {}, {} } };
}

UserCodeValue::UserCodeValue( std::string userName, std::string userCode, std::string mode )
: Value( ValueType::Enum::UserCode )
, m_userName( std::move( userName ) )
, m_userCode( std::move( userCode ) )
, m_userMode( std::move( mode ) ) {}

UserCodeValue::~UserCodeValue() = default;

Json UserCodeValue::toJson() const {
    using namespace Resources;

    auto result = Json::object();

    result[ UsercodeValueFields::UserNameFieldName ] = m_userName;
    result[ UsercodeValueFields::UserCodeFieldName ] = m_userCode;

    if( !m_userMode.empty() ) {
        result[ UsercodeValueFields::UserModeFieldName ] = m_userMode;
    }

    return result;
}

bool UserCodeValue::isEqual( const Value& other ) const {
    const auto& otherUserCodeValue = valueCast< UserCodeValue >( other );
    return m_userName == otherUserCodeValue.m_userName && m_userCode == otherUserCodeValue.m_userCode &&
           m_userMode == otherUserCodeValue.m_userMode;
}

void UserCodeValue::set( const Value& other ) {
    const auto& otherUserCodeValue = valueCast< UserCodeValue >( other );
    m_userName = otherUserCodeValue.m_userName;
    m_userCode = otherUserCodeValue.m_userCode;
    m_userMode = otherUserCodeValue.m_userMode;
}

void UserCodeValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void UserCodeValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

const std::string& UserCodeValue::getUserCode() const {
    return m_userCode;
}

void UserCodeValue::setUserCode( const std::string& userCode ) {
    m_userCode = userCode;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
