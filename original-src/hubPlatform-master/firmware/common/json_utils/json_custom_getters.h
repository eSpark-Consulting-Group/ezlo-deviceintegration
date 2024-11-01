#pragma once

#include "json_utils/json_custom_getters_details.h"

namespace JsonCustomGetters {
// clang-format off
/* Custom value getters
 * Helper method that allow to read any data (including vectors, structs and their combinations) from json
 *
 * Sample (reading json array to vector of structs with optional name field):
 * struct Struct {
 *     int id;
 *     std::string name;
 * };
 * const auto getter = makeVectorGetter(makeStructGetter(makeMemberMapping(&Struct::id, "id"),
 *                                      makeMemberMapping(&Struct::name, "name", OptionalMemberPolicy())));
 * std::vector<Struct> result;
 * if (getter(json, result)) {
 *    // success
 * }
 *
 * For more samples look at UTs
 */
// clang-format on

using Details::GettingResult;

// Custom value getters

template < typename WrappedGetter, typename ValidationPolicy >
inline Details::Validatior< std::decay_t< WrappedGetter >, std::decay_t< ValidationPolicy > > makeValidator(
    WrappedGetter&& wrappedGetter,
    ValidationPolicy&& validationPolicy ) {
    return { std::forward< WrappedGetter >( wrappedGetter ), std::forward< ValidationPolicy >( validationPolicy ) };
}

inline Details::KeyGetter makeKeyGetter() {
    return {};
}

template < typename T >
Details::ValueGetter< T > makeDefaultGetter() {
    return {};
}

template < typename... FieldGetters >
inline Details::StructGetter< std::decay_t< FieldGetters >... > makeStructGetter( FieldGetters&&... fieldGetters ) {
    return { std::make_tuple( std::forward< FieldGetters >( fieldGetters )... ) };
}

template < typename ValueTypeGetter >
Details::VectorGetter< std::decay_t< ValueTypeGetter > > makeVectorGetter( ValueTypeGetter&& valueGetter ) {
    return { std::forward< ValueTypeGetter >( valueGetter ) };
}

template < typename ValueGetter >
Details::FieldGetter< std::decay_t< ValueGetter > > makeFieldGetter( const char* fieldKey, ValueGetter&& valueGetter ) {
    return { fieldKey, std::forward< ValueGetter >( valueGetter ) };
}

template < typename ValueGetter, typename ValidationPolicy >
auto makeFieldGetter( const char* fieldKey, ValueGetter&& valueGetter, ValidationPolicy&& policy )
    -> decltype( makeValidator( makeFieldGetter( fieldKey, valueGetter ), policy ) ) {
    return makeValidator( makeFieldGetter( fieldKey, std::forward< ValueGetter >( valueGetter ) ),
                          std::forward< ValidationPolicy >( policy ) );
}

// mappings to struct members

template < typename Type, typename StructType, typename FieldGetter >
inline Details::MemberMapping< StructType, std::decay_t< FieldGetter > > makeMemberMapping(
    Type StructType::*member,
    FieldGetter&& fieldGetter ) {
    return { member, std::forward< FieldGetter >( fieldGetter ) };
}

template < typename Type, typename StructType, typename... ValidationPolicyOpt >
inline auto makeMemberMapping( Type StructType::*member, const char* fieldKey, ValidationPolicyOpt&&... policyOpt )
    -> decltype( makeMemberMapping( member, makeFieldGetter( fieldKey, makeDefaultGetter< Type >(), policyOpt... ) ) ) {
    return makeMemberMapping(
        member,
        makeFieldGetter( fieldKey, makeDefaultGetter< Type >(), std::forward< ValidationPolicyOpt >( policyOpt )... ) );
}

// Default validation policies

struct OptionalMemberPolicy {
    OptionalMemberPolicy( bool shouldResetValue = true )
    : m_shouldResetValue( shouldResetValue ) {}

    template < typename T >
    GettingResult operator()( GettingResult result, T& value ) const {
        if( result.m_error == GettingResult::Error::Default && m_shouldResetValue ) {
            value = {};
        }
        return result.m_error == GettingResult::Error::Default ?
                   GettingResult( GettingResult::Error::OptionalStructMember ) :
                   result;
    }

    bool m_shouldResetValue;
};

struct CriticalErrorPolicy {
    template < typename T >
    GettingResult operator()( GettingResult result, const T& value ) const {
        return result ? result : GettingResult( GettingResult::Error::Critical );
    }
};

struct CheckJsonStringStrictPolicy {
    GettingResult operator()( GettingResult result, const std::string& str ) const {
        try {
            jsonCheckString( str );
            return {};
        }
        catch( const std::exception& ) {
            return GettingResult::Error::Critical;
        }
    }
};

}  // namespace JsonCustomGetters
