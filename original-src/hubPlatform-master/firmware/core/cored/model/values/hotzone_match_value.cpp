#include "cored/model/values/hotzone_match_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"
#include "common_utils/exception_utils.h"

#include <map>

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace {
const std::map< std::string, HotzoneMatchValue::MatchPolicy >& getPolicyMap() {
    using Policy = HotzoneMatchValue::MatchPolicy;
    static const std::map< std::string, Policy > map = { { "any", Policy::Any },       { "anyOf", Policy::AnyOf },
                                                         { "allOf", Policy::AllOf },   { "oneOf", Policy::OneOf },
                                                         { "noneOf", Policy::NoneOf }, { "none", Policy::None } };
    return map;
};

bool isMatchListRequired( HotzoneMatchValue::MatchPolicy policy ) {
    using Policy = HotzoneMatchValue::MatchPolicy;
    return ( policy == Policy::AllOf || policy == Policy::AnyOf || policy == Policy::OneOf ||
             policy == Policy::NoneOf );
}

}  // namespace

std::unique_ptr< HotzoneMatchValue > HotzoneMatchValue::createValue( const Json& value ) {
    using namespace Resources;

    if( !value.is_object() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::HotzoneMatch, "value is not an object" };

    MatchPolicy policy = MatchPolicy::Any;
    MatchList list;

    try {
        auto& policyMap = getPolicyMap();
        policy = policyMap.at( json_get_str( value.at( HotzoneMatchValueFields::Match ) ) );

        const bool listRequired = isMatchListRequired( policy );
        auto listIt = value.find( HotzoneMatchValueFields::List );
        const bool listFound = listIt != value.cend();

        if( ( listRequired && !listFound ) || ( !listRequired && listFound ) )
            throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::HotzoneMatch, "invalid \"list\" usage" };

        if( listFound ) {
            if( !listIt->is_array() )
                throw UnexpectedJsonValueFormatError{
                    value, ValueType::Enum::HotzoneMatch, "\"list\" is not an array"
                };

            for( auto& element : *listIt ) {
                auto id = json_get_str( element );
                if( id.empty() )
                    throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::HotzoneMatch, "invalid id" };
                list.emplace( id );
            }

            if( list.empty() )
                throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::HotzoneMatch, "empty list" };
        }
    }
    catch( const std::exception& e ) {
        throw UnexpectedJsonValueFormatError{
            value, ValueType::Enum::HotzoneMatch, "exception(" + CommonUtils::whatError( e ) + ")"
        };
    }
    catch( ... ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::HotzoneMatch, "unknown exception" };
    }

    return std::unique_ptr< HotzoneMatchValue >{ new HotzoneMatchValue{ policy, std::move( list ) } };
}

std::unique_ptr< HotzoneMatchValue > HotzoneMatchValue::createDefaultValue() {
    return std::unique_ptr< HotzoneMatchValue >{ new HotzoneMatchValue{ MatchPolicy::Any, {} } };
}

HotzoneMatchValue::~HotzoneMatchValue() = default;

Json HotzoneMatchValue::toJson() const {
    namespace Fields = Resources::HotzoneMatchValueFields;
    if( !isMatchListRequired( m_matchPolicy ) )
        return JSON( Fields::Match << matchPolicyToStr( m_matchPolicy ) );

    JsonArray list;
    for( auto& id : m_matchList )
        list.push_back( id );

    return JSON( Fields::Match << matchPolicyToStr( m_matchPolicy ) << Fields::List << std::move( list ) );
}

void HotzoneMatchValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void HotzoneMatchValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool HotzoneMatchValue::isEqual( const Value& other ) const {
    const auto& object = valueCast< HotzoneMatchValue >( other );
    return m_matchPolicy == object.m_matchPolicy && m_matchList == object.m_matchList;
}

void HotzoneMatchValue::set( const Value& other ) {
    const auto& object = valueCast< HotzoneMatchValue >( other );
    m_matchPolicy = object.m_matchPolicy;
    m_matchList = object.m_matchList;
}

HotzoneMatchValue::HotzoneMatchValue( MatchPolicy matchPolicy, MatchList matchList )
: Value( ValueType::Enum::HotzoneMatch )
, m_matchPolicy( matchPolicy )
, m_matchList( std::move( matchList ) ) {}

HotzoneMatchValue::MatchPolicy HotzoneMatchValue::getMatchPolicy() const {
    return m_matchPolicy;
}

const HotzoneMatchValue::MatchList& HotzoneMatchValue::getMatchList() const {
    return m_matchList;
}

std::string HotzoneMatchValue::matchPolicyToStr( MatchPolicy policy ) {
    auto& map = getPolicyMap();
    for( auto& element : map ) {
        if( policy == element.second )
            return element.first;
    }
    return {};
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
