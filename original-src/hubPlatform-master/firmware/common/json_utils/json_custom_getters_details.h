#pragma once

#include "base_utils/tuple_utils.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace JsonCustomGetters {

namespace Details {

constexpr const bool IsLoggingEnabledByDefault = true;

struct GettingResult {
    enum class Error { None, OptionalStructMember, Default, Critical };

    GettingResult( Error err = Error::None )
    : m_error( err ) {}

    operator bool() const {
        return m_error == Error::None;
    }

    Error m_error;
};

inline JsonObject getObject( JsonObject&& object ) {
    return std::move( object );
}

inline const JsonObject& getObject( const JsonObject& object ) {
    return object;
}

inline const JsonObject& getObject( const JsonObject::const_iterator& it ) {
    return it.value();
}

template < typename ValueTypeGetter >
struct VectorGetter {
    using Type = std::vector< typename ValueTypeGetter::Type >;

    template < typename ObjectOrIterator >
    GettingResult operator()( const ObjectOrIterator& objectOrIterator,
                              Type& vec,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        const auto& obj = getObject( objectOrIterator );

        if( vec.empty() ) {
            vec.reserve( obj.size() );
        }

        GettingResult result( GettingResult::Error::Default );

        for( auto it = obj.begin(); it != obj.end(); ++it ) {
            typename ValueTypeGetter::Type value;

            const auto currentResult = m_valueGetter( it, value, isLoggingEnabled );

            if( currentResult ) {
                vec.push_back( std::move( value ) );
                result = {};
            }
            else if( currentResult.m_error == GettingResult::Error::Critical ) {
                return currentResult;
            }
        }

        return result;
    }

    ValueTypeGetter m_valueGetter;
};

struct KeyGetter {
    using Type = std::string;

    GettingResult operator()( const JsonObject::const_iterator& it,
                              Type& str,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        try {
            str = it.key();
            return {};
        }
        catch( const std::exception& ) {
            if( isLoggingEnabled ) {
                Log::info( std::string( "Trying to read key from incompatible json field" ) );
            }
            return GettingResult::Error::Default;
        }
    }
};

template < typename T >
struct ValueGetter {
    using Type = T;

    template < typename ObjectOrIterator >
    GettingResult operator()( const ObjectOrIterator& objectOrIterator,
                              Type& val,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        const JsonObject& obj = getObject( objectOrIterator );
        try {
            val = obj.get< Type >();
            return {};
        }
        catch( const std::exception& ) {
            if( isLoggingEnabled ) {
                Log::info( std::string( "Cannot read json field of type " ) + typeid( Type ).name() );
            }
            return GettingResult::Error::Default;
        }
    }
};

template < typename T >
struct ValueGetter< std::vector< T > > : VectorGetter< ValueGetter< T > > {};

template < typename FieldsGetter, typename... FieldsGetters >
struct StructGetter {
    using Type = typename FieldsGetter::Struct;

    template < typename ObjectOrIterator >
    struct MappingHandler {
        template < typename Mapping >
        void operator()( Mapping&& mapping ) {
            const auto member = mapping.m_member;
            auto fieldResult = mapping.m_fieldGetter( m_objectOrIterator, m_val.*member, m_isLoggingEnabled );

            if( fieldResult.m_error == GettingResult::Error::OptionalStructMember ) {
                fieldResult.m_error = GettingResult::Error::None;
            }

            if( static_cast< std::underlining_type_t< GettingResult::Error > >( fieldResult.m_error ) >
                static_cast< std::underlining_type_t< GettingResult::Error > >( m_result.m_error ) ) {
                m_result = fieldResult;
            }
        }

        const ObjectOrIterator& m_objectOrIterator;
        Type& m_val;
        GettingResult& m_result;
        bool m_isLoggingEnabled;
    };

    template < typename ObjectOrIterator >
    GettingResult operator()( const ObjectOrIterator& objectOrIterator,
                              Type& val,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        if( !getObject( objectOrIterator ).is_object() ) {
            return GettingResult::Error::Default;
        }

        GettingResult result;
        foreachInTuple( m_getters,
                        MappingHandler< ObjectOrIterator >{ objectOrIterator, val, result, isLoggingEnabled } );

        return result;
    }

    std::tuple< FieldsGetter, FieldsGetters... > m_getters;
};

template < typename ValueGetter >
struct FieldGetter {
    using Type = typename ValueGetter::Type;

    template < typename ObjectOrIterator >
    GettingResult operator()( const ObjectOrIterator& objectOrIterator,
                              Type& val,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        const auto& obj = getObject( objectOrIterator );
        if( !obj.is_object() ) {
            return GettingResult::Error::Default;
        }

        auto it = obj.find( m_key );
        if( it == obj.end() ) {
            return GettingResult::Error::Default;
        }

        return m_valueGetter( it, val, isLoggingEnabled );
    }

    const char* m_key;
    ValueGetter m_valueGetter;
};

template < typename WrappedGetter, typename ValidationPolicy >
struct Validatior {
    using Type = typename WrappedGetter::Type;

    template < typename ObjectOrIterator >
    GettingResult operator()( const ObjectOrIterator& objectOrIterator,
                              Type& val,
                              bool isLoggingEnabled = IsLoggingEnabledByDefault ) const {
        const auto result = m_wrappedGetter( objectOrIterator, val, isLoggingEnabled );
        return result.m_error == GettingResult::Error::Critical ? result : m_validationPolicy( result, val );
    }

    WrappedGetter m_wrappedGetter;
    ValidationPolicy m_validationPolicy;
};

template < typename StructType, typename FieldGetter >
struct MemberMapping {
    using Struct = StructType;
    using Type = typename FieldGetter::Type;

    Type StructType::*m_member;
    FieldGetter m_fieldGetter;
};

}  // namespace Details
}  // namespace JsonCustomGetters
