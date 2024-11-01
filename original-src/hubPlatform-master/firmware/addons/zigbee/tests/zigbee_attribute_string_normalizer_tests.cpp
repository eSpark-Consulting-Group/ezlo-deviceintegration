#include "zigbee/utils/attribute_string_normalizer.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zigbee {
namespace UT {
using namespace ::testing;

class ZigbeeAttributeStringNormalizerTest : public Test {
public:
    void test( std::string source, std::string result ) {
        std::string data = source;
        AttributeStringNormalizer::normalize( data );
        EXPECT_EQ( data, result );
    }

    void testNoChanges( std::string s ) {
        test( s, s );
    }
};

TEST_F( ZigbeeAttributeStringNormalizerTest, testNoChanges_1 ) {
    testNoChanges( "simpleTest" );
    testNoChanges( "test with spaces 123" );
    testNoChanges( "test.with.dots." );
    testNoChanges( "test-with_delimeters.+" );
}

TEST_F( ZigbeeAttributeStringNormalizerTest, testDeleteUnexpectedCharacters_2 ) {
    test( "\ttest \nwith\b controls\r\f", "test with controls" );
    test( "#test #with hash", "test with hash" );
    test( ".test #with ,delimeters;", ".test with delimeters" );
}

TEST_F( ZigbeeAttributeStringNormalizerTest, testRealDevices_3 ) {
    testNoChanges( "AXIS_Gear" );
    testNoChanges( "CentraLite_3140-C" );
    testNoChanges( "Centralite_3157100" );
    testNoChanges( "CentraLite_3300-Geu" );
    testNoChanges( "CentraLite_3305-Geu" );
    testNoChanges( "CentraLite_3310-G" );
    testNoChanges( "CentraLite_3310-Geu" );
    testNoChanges( "CentraLite_3315-G" );
    testNoChanges( "CentraLite_3320-L" );
    testNoChanges( "CentraLite_3323-D" );
    testNoChanges( "CentraLite_3323-G" );
    testNoChanges( "CentraLite_3323-Geu" );
    testNoChanges( "CentraLite_3326-L" );
    testNoChanges( "CentraLite_3328-G" );
    testNoChanges( "CentraLite_3400" );
    testNoChanges( "CentraLite_3400-G" );
    testNoChanges( "Centralite Systems_3131-G" );
    testNoChanges( "Centralite Systems_4200-C" );
    testNoChanges( "innr_SP 222" );
    testNoChanges( "innr_SP 224" );
    testNoChanges( "Jasco Products_43076" );
    testNoChanges( "Jasco Products_43084" );
    testNoChanges( "Jasco Products_43090" );
    testNoChanges( "Jasco Products_43094" );
    testNoChanges( "Jasco Products_43102" );
    testNoChanges( "Jasco Products_43132" );
    testNoChanges( "Jasco Products_45853" );
    testNoChanges( "Jasco Products_45856" );
    testNoChanges( "Jasco Products_45857" );
    testNoChanges( "LUMI_lumi.remote.b1acn01" );
    testNoChanges( "Lutron_Z3-1BRL" );
    testNoChanges( "Philips_LOM004" );
    testNoChanges( "Samjin_multi" );
    testNoChanges( "Samjin_outlet" );
    testNoChanges( "sengled_E12-N14" );
    testNoChanges( "sengled_E12-N1E" );
    testNoChanges( "sengled_E12-N1EA" );
    testNoChanges( "sengled_E1C-NB6" );
}

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
