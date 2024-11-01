#include "json_utils/json_approximate_size.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class JsonApproximateSizeTests : public ::testing::Test {};

TEST_F( JsonApproximateSizeTests, objects ) {
    Json source = { { "aaaa", "0123" }, { "0123", "dddd" } };

    auto size = jsonApproximateSize( source );
    auto dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    source[ "long_key" ] = "And a long string added too";
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    // escaped string
    source[ "long_key" ] = "\"And\" \'a\' lot of escaped characters \t\n";
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    // booleans
    source = { { "true", true }, { "false", false } };
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    // numbers
    source = { { "signed_int", -1234567 }, { "unsigned_int", 123456 }, { "zero", 0 }, { "float", -27.0004 } };
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) +
               ", dump: " + dump );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";
}

TEST_F( JsonApproximateSizeTests, arrays ) {
    Json source = { "aaaa", "0123", "0123", "dddd" };

    auto size = jsonApproximateSize( source );
    auto dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    // numbers
    source = { -1234567, 123456, 0, -27.0004 };
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) +
               ", dump: " + dump );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";

    // booleans
    source = { true, true, false, true, false };
    size = jsonApproximateSize( source );
    dump = source.dump();
    Log::info( "Calculated size: " + cc::num10( size ) + ", dump size: " + cc::num10( dump.size() ) +
               ", dump: " + dump );
    ASSERT_LE( abs( (int) size - dump.size() ), dump.size() * 0.1 ) << "Error less than 10 percent";
}

}  // namespace UT
}  // namespace HubPlatform