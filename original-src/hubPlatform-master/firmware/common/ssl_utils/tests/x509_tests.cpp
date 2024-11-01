#include "ssl_utils/x509.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1 Extract validity period
DONE        1.1 Correct certificate
DONE        1.2 Broken certificate
DONE    2 Compare
DONE        2.1 Two ASN.1 times
DONE        2.2 Now and future ASN.1 time
DONE        2.3 Now and past ASN.1 time
DONE        2.4 Now and Now
DONE        2.5 Broken ASN.1 time
DONE    3 Choose
DONE        3.1 No periods
DONE        3.2 No valid periods
DONE        3.3 Broken periods
DONE        3.4 Single valid period
DONE        3.5 Multiple valid periods
DONE        3.6 Using now as a reference
DONE    4 Compare
DONE        4.1 Two ASN.1 times
DONE        4.2 Two distant ASN.1 times
DONE        4.3 Now and Now
DONE        4.4 Broken ASN.1 time

*/

using namespace ::testing;
using namespace HubPlatform::SslUtils;

class X509TestSuite : public Test {};

namespace {
std::string operator"" _y( unsigned long long relativeIndex ) {
    if( relativeIndex < 0 || relativeIndex > 999 ) {
        throw std::out_of_range( "Use relativeIndex from range [0;999]" );
    }
    std::stringstream ss;
    ss << "2" << std::setw( 3 ) << std::setfill( '0' ) << relativeIndex << "0101000000Z";
    return ss.str();
}
}  // namespace

TEST_F( X509TestSuite, testExtractValidityPeriodFromCorrectCertificate /*1.1*/ ) {
    std::string certificate = "-----BEGIN CERTIFICATE-----\n"
                              "MIICCjCCAbCgAwIBAgICOPAwCgYIKoZIzj0EAwIwgZAxCzAJBgNVBAYTAlVTMRQw\n"
                              "EgYDVQQIDAsgTmV3IEplcnNleTEQMA4GA1UEBwwHQ2xpZnRvbjEPMA0GA1UECgwG\n"
                              "SVQgT3BzMQ8wDQYDVQQLDAZJVCBPcHMxFDASBgNVBAMMC2VaTE8gTFREIENBMSEw\n"
                              "HwYJKoZIhvcNAQkBFhJzeXNhZG1pbnNAZXpsby5jb20wIBcNMTkwNjA0MDk1OTE2\n"
                              "WhgPMjI5MzAzMTgwOTU5MTZaMIGLMQswCQYDVQQGEwJVUzETMBEGA1UECAwKTmV3\n"
                              "IEplcnNleTEQMA4GA1UEBwwHQ2xpZnRvbjETMBEGA1UECgwKY29udHJvbGxlcjEt\n"
                              "MCsGA1UECwwkNjliMTU5NjAtODZhZi0xMWU5LTgzMmItNmQ0ZTQyYThjMTU4MREw\n"
                              "DwYDVQQDDAg3MDAxMDE5NTBWMBAGByqGSM49AgEGBSuBBAAKA0IABPU9WJBVLReJ\n"
                              "Dir60CAeoHHVu05WuAiKGzxRmGHx3vGibZ4MYSx0u0BvtEhNlFMaHQfq4TGrmBf/\n"
                              "MwZrpDT9cjwwCgYIKoZIzj0EAwIDSAAwRQIhANU02HkFROghwbxMLtzMlQRZ2sZG\n"
                              "rOvhEd2RSvO+/CJxAiByMMQK/ScieECPLX/+TRuVq3g5Qc1vQpZ0gL8vPawlPQ==\n"
                              "-----END CERTIFICATE-----";
    std::string notBefore;
    std::string notAfter;
    std::tie( notBefore, notAfter ) = getValidityPeriod( certificate );

    EXPECT_EQ( notBefore, "190604095916Z" );
    EXPECT_EQ( notAfter, "22930318095916Z" );
}

TEST_F( X509TestSuite, testExtractValidityPeriodFromBrokenCertificate /*1.2*/ ) {
    std::string certificate = "garbage";
    std::string notBefore;
    std::string notAfter;
    std::tie( notBefore, notAfter ) = getValidityPeriod( certificate );

    EXPECT_EQ( notBefore, "" );
    EXPECT_EQ( notAfter, "" );
}

TEST_F( X509TestSuite, testCompareTwoAsn1Times /*2.1*/ ) {
    std::string first = "190604095916Z";
    std::string second = "22930318095916Z";

    EXPECT_TRUE( asn1TimeLess( first, second ) );
    EXPECT_FALSE( asn1TimeLess( second, first ) );
}

TEST_F( X509TestSuite, testCompareNowAndFutureAsn1Time /*2.2*/ ) {
    std::string now = "";
    std::string future = "32930318095916Z";

    EXPECT_TRUE( asn1TimeLess( now, future ) );
    EXPECT_FALSE( asn1TimeLess( future, now ) );
}

TEST_F( X509TestSuite, testCompareNowAndPastAsn1Time /*2.3*/ ) {
    std::string past = "19170604095916Z";
    std::string now = "";

    EXPECT_TRUE( asn1TimeLess( past, now ) );
    EXPECT_FALSE( asn1TimeLess( now, past ) );
}

TEST_F( X509TestSuite, testCompareNowAndNow /*2.4*/ ) {
    std::string now = "";

    EXPECT_FALSE( asn1TimeLess( now, now ) );
}

TEST_F( X509TestSuite, testCompareBrokenAsn1Time /*2.5*/ ) {
    std::string now = "";
    std::string broken = "garbage";

    EXPECT_TRUE( asn1TimeLess( now, broken ) );
    EXPECT_TRUE( asn1TimeLess( broken, now ) );
}

TEST_F( X509TestSuite, testChooseNoPeriods /*3.1*/ ) {
    int chosen = chooseValidPeriod( {}, 10_y );

    EXPECT_EQ( chosen, -1 );
}

TEST_F( X509TestSuite, testChooseNoValidPeriods /*3.2*/ ) {
    int chosen = chooseValidPeriod( { { 5_y, 15_y }, { 25_y, 30_y } }, 20_y );

    EXPECT_EQ( chosen, -1 );
}

TEST_F( X509TestSuite, testChooseBrokenPeriods /*3.3*/ ) {
    int chosen = chooseValidPeriod( { { 15_y, 5_y }, { "??", 30_y } }, 20_y );

    EXPECT_EQ( chosen, -1 );
}

TEST_F( X509TestSuite, testChooseSingleValidPeriod /*3.4*/ ) {
    int chosen = chooseValidPeriod( { { 5_y, 15_y }, { 20_y, 25_y }, { 30_y, 40_y } }, 30_y );

    EXPECT_EQ( chosen, 2 );
}

TEST_F( X509TestSuite, testChooseMultipleValidPeriods /*3.5*/ ) {
    int chosen = chooseValidPeriod( { { 5_y, 30_y }, { 15_y, 35_y }, { 10_y, 35_y } }, 20_y );

    EXPECT_EQ( chosen, 1 );
}

TEST_F( X509TestSuite, testChooseUsingNowAsAReference /*3.6*/ ) {
    int chosen = chooseValidPeriod( { { 0_y, 99_y } } );

    EXPECT_EQ( chosen, 0 );
}

TEST_F( X509TestSuite, testDiffTwoAsn1Times /*4.1*/ ) {
    std::string first = "20200128100000Z";
    std::string second = "20200128110000Z";

    int days = -1, seconds = -1;
    EXPECT_TRUE( asn1TimeDiff( days, seconds, first, second ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( 0, 3600 ) );
    EXPECT_TRUE( asn1TimeDiff( days, seconds, second, first ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( 0, -3600 ) );
}

TEST_F( X509TestSuite, testDiffTwoDistantAsn1Times /*4.2*/ ) {
    std::string first = "190604095916Z";
    std::string second = "22930318095958Z";

    int days = -1, seconds = -1;
    EXPECT_TRUE( asn1TimeDiff( days, seconds, first, second ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( 99999, 42 ) );
    EXPECT_TRUE( asn1TimeDiff( days, seconds, second, first ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( -99999, -42 ) );
}

TEST_F( X509TestSuite, testDiffNowAndNow /*4.3*/ ) {
    std::string now = "";

    int days = -1, seconds = -1;
    EXPECT_TRUE( asn1TimeDiff( days, seconds, now, now ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( 0, 0 ) );
}

TEST_F( X509TestSuite, testDiffBrokenAsn1Time /*4.4*/ ) {
    std::string now = "";
    std::string broken = "garbage";

    int days = -1, seconds = -1;
    EXPECT_FALSE( asn1TimeDiff( days, seconds, now, broken ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( -1, -1 ) );
    EXPECT_FALSE( asn1TimeDiff( days, seconds, broken, now ) );
    EXPECT_EQ( std::make_pair( days, seconds ), std::make_pair( -1, -1 ) );
}
