#pragma once

#include <gtest/gtest.h>
#include "json_utils/json_fwd.h"

#define EXPECT_THROW_WITH_DESCRIPTION( statement, exceptionName, description ) \
    try {                                                                      \
        statement();                                                           \
        ADD_FAILURE() << "statement has to throw an exception";                \
    }                                                                          \
    catch( const exceptionName& error ) {                                      \
        EXPECT_EQ( std::string( error.what() ), exceptionMsg );                \
    }                                                                          \
    catch( ... ) {                                                             \
        ADD_FAILURE() << "incorrect type of an exception";                     \
    }

#define ASSERT_THROW_WITH_DESCRIPTION( statement, exceptionName, description ) \
    try {                                                                      \
        statement();                                                           \
        FAIL() << "statement has to throw an exception";                       \
    }                                                                          \
    catch( const exceptionName& error ) {                                      \
        ASSERT_EQ( std::string( error.what() ), description );                 \
    }                                                                          \
    catch( ... ) {                                                             \
        FAIL() << "incorrect type of an exception";                            \
    }

#define MAKE_MESSAGE( message ) ( std::string{ __FILE__ } + ":" + std::to_string( __LINE__ ) + " " + ( message ) )

namespace HubPlatform {
namespace Tests {

using KeyStringList = std::vector< std::pair< std::string, std::string > >;

inline std::string replaceTokensInString( std::string str, const KeyStringList& replaceList ) {
    for( const auto& replacePair : replaceList ) {
        const std::string& toReplace = replacePair.first;
        const std::string& replaceWith = replacePair.second;

        size_t pos = str.find( toReplace );
        while( pos != std::string::npos ) {
            str.replace( pos, toReplace.size(), replaceWith );
            pos = str.find( toReplace, pos + 1 );
        }
    }
    return str;
}

inline JsonObject replaceTokens( std::string json, const KeyStringList& replaceList ) {
    std::string transformed = replaceTokensInString( std::move( json ), replaceList );
    try {
        return JsonObject::parse( transformed );
    }
    catch( ... ) {
        ADD_FAILURE() << "Unable to parse json with replaced tokens: " << transformed;
        throw;
    }
}

inline JsonObject replaceTokens( const JsonObject& json, const KeyStringList& replaceList ) {
    return replaceTokens( json.dump(), replaceList );
}

inline std::string quote( const std::string& str ) {
    return "\"" + str + "\"";
}

}  // namespace Tests
}  // namespace HubPlatform
