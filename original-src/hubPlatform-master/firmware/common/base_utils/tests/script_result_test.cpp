#include "base_utils/script_result.h"

#include <gtest/gtest.h>

namespace HubPlatform {
static bool operator==( const HubPlatform::ScriptResult& lhs, const HubPlatform::ScriptResult& rhs ) {
    if( lhs.hasError() != rhs.hasError() )
        return false;

    const int lhsValuesCount = lhs.getValuesCount();
    const int rhsValuesCount = rhs.getValuesCount();
    if( lhsValuesCount != rhsValuesCount )
        return false;

    for( int i = 0; i < lhsValuesCount; ++i ) {
        if( lhs.getValue( i ) != rhs.getValue( i ) )
            return false;
    }

    return true;
}

static void PrintTo( const HubPlatform::ScriptResult& result, ::std::ostream* os ) {
    *os << result.toJson();
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace UT {

TEST( ScriptResultToFromJson, validResult ) {
    const ScriptResult result( ScriptResult::Values{ "1", true } );

    const auto joResult = result.toJson();

    const auto sameResult = ScriptResult::fromJson( joResult );

    EXPECT_EQ( result, sameResult );
}

TEST( ScriptResultToFromJson, errorResult ) {
    const ScriptResult result( "error message" );

    const auto joResult = result.toJson();

    const auto sameResult = ScriptResult::fromJson( joResult );

    EXPECT_EQ( result, sameResult );
}

}  // namespace UT
}  // namespace HubPlatform
