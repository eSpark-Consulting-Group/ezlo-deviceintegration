#include "http/multipart_boundary_processor.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Process headers
DONE        1.1 No marker
DONE        1.2 Boundary in wrong header
DONE        1.3 Boundary in correct header but w/o multipart
DONE        1.4 No boundary in correct header with multipart
DONE        1.5 Boundary before multipart in correct header
DONE        1.6 Empty boundary
DONE        1.7 Proper boundary
DONE        1.8 Case-insensitive search
DONE    2. Last boundary w/o Content-Length
DONE        2.1 No boundary in batches
DONE        2.2 Single batch
DONE        2.3 Boundary crosses batches
DONE        2.4 Lots of small batches
DONE        2.5 Multiple parts
DONE        2.6 Multiple parts in multiple batches
DONE        2.7 No multipart boundary header
DONE        2.8 Only boundary in batch
DONE    3. Last headers
DONE        3.1 No headers in batches
DONE        3.2 Single batch
DONE        3.3 Headers end crosses batches
DONE        3.4 Lots of small batches
DONE        3.5 Multiple parts
DONE        3.6 Multiple parts in multiple batches
DONE        3.7 No multipart boundary header
DONE    4. Content-Length
DONE        4.1 No Content-Length
DONE        4.2 Malformed Content-Length
DONE        4.3 Single Content-Length
DONE        4.4 Multiple Content-Lengths
DONE    5. Last boundary with Content-Length
DONE        5.1 Part with Content-Length is last
DONE        5.2 Part with Content-Length is not last
DONE        5.3 Part with Content-Length followed by incomplete headers
DONE        5.4 Part with Content-Length followed by incomplete body

 */

using namespace ::testing;

namespace HubPlatform {
namespace UT {

using namespace CommonHttp;

class MultipartBoundaryTests : public Test {
public:
    static void pushBytes( MultipartBoundaryProcessor& mbp, const std::string& batch ) {
        mbp.processBytes( batch.c_str(), batch.size() );
    }
};

TEST_F( MultipartBoundaryTests, ProcessHeadersNoMarker /* 1.1 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "MIME-Version", "1.0" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersBoundaryInWrongHeader /* 1.2 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "MIME-Version", "multipart/mixed; boundary=hikboundary" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersBoundaryInCorrectHeaderButWOMultipart /* 1.3 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "boundary=hikboundary" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersNoBoundaryInCorrectHeaderWithMultipart /* 1.4 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersBoundaryBeforeMultipartInCorrectHeader /* 1.5 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "boundary=hikboundary; multipart/mixed" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersEmptyBoundary /* 1.6 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "--" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersProperBoundary /* 1.7 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "--hikboundary" );
}

TEST_F( MultipartBoundaryTests, ProcessHeadersCaseInsensitiveSearch /* 1.8 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "CoNtEnT-TyPe", "multiPART/mixed; bouNDary=hikboundary" );

    EXPECT_EQ( mbp.getBoundaryMarker(), "--hikboundary" );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthNoBoundaryInBatches /* 2.1 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<activePostCount>0</activePostCount>\r\n"
               "<eventType>HDS</eventType>\r\n"
               "<eventState>active</eventState>" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), -1 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), -1 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthSingleBatch /* 2.2 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 101 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthBoundaryCrossesBatches /* 2.3 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n" );
    pushBytes( mbp,
               "</EventNotificationAlert>\r\n"
               "--hik" );
    pushBytes( mbp,
               "boundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 101 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthBoundaryLotsOfSmallBatches /* 2.4 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    std::string body = "<channelID>1</channelID>\r\n"
                       "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
                       "</EventNotificationAlert>\r\n"
                       "--hikboundary\r\n"
                       "Content-Type: application/xml; charset=\"UTF-8\"";

    for( auto& byte : body ) {
        mbp.processBytes( &byte, 1 );
    }

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 101 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthMultipleParts /* 2.5 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "--hikboundary\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "--hikboundary\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 144 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 131 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthMultiplePartsInMultipleBatches /* 2.6 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "--hikboundary\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "--hikboundary\r\n"
               "</Event" );

    pushBytes( mbp,
               "NotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "--hik" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 144 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 131 );

    pushBytes( mbp,
               "boundary\r\n"
               "</EventNotificationAlert>\r\n"
               "--hik" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 207 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 194 );

    pushBytes( mbp,
               "boundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 249 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 236 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthNoMultipartBoundaryHeader /* 2.7 */ ) {
    MultipartBoundaryProcessor mbp;

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), -1 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), -1 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastBoundaryWOContentLengthOnlyBoundaryInBatch /* 2.8 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp, "--hikboundary" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 13 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 0 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndNoHeadersInBatches /* 3.1 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndSingleBatch /* 3.2 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 187 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndHeadersEndCrossesBatches /* 3.3 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hik" );

    pushBytes( mbp,
               "boundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n" );

    pushBytes( mbp,
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 187 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndBoundaryLotsOfSmallBatches /* 3.4 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    std::string body = "<channelID>1</channelID>\r\n"
                       "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
                       "</EventNotificationAlert>\r\n"
                       "--hikboundary\r\n"
                       "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
                       "Content-Length: 502\r\n"
                       "\r\n"
                       "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">";

    for( auto& byte : body ) {
        mbp.processBytes( &byte, 1 );
    }

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 187 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndMultipleParts /* 3.5 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 185 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 258 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndMultiplePartsInMultipleBatches /* 3.6 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "--hik" );

    pushBytes( mbp,
               "boundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 185 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 258 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndNoMultipartBoundaryHeader /* 3.7 */ ) {
    MultipartBoundaryProcessor mbp;

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), -1 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), -1 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), -1 );
}

TEST_F( MultipartBoundaryTests, LastHeadersEndEmptyPartHeaders /* 3.8 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">" );

    EXPECT_EQ( mbp.getLastBoundaryStart(), 114 );
    EXPECT_EQ( mbp.getLastBoundaryEnd(), 101 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 118 );
    EXPECT_EQ( mbp.getLastHeadersEnd(), 118 );
}

TEST_F( MultipartBoundaryTests, ContentLengthNoContentLength /* 4.1 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getContentLength(), -1 );
}

TEST_F( MultipartBoundaryTests, ContentLengthMalformedContentLength /* 4.2 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: five\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getContentLength(), -1 );
}

TEST_F( MultipartBoundaryTests, ContentLengthSingleContentLength /* 4.3 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Length: 502\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getContentLength(), 502 );
}

TEST_F( MultipartBoundaryTests, ContentLengthMultipleContentLengths /* 4.4 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-07-24T03:45:15--8:00</dateTime>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Length: 1648\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n" );

    EXPECT_EQ( mbp.getContentLength(), 502 );
}

TEST_F( MultipartBoundaryTests, PartWithContentLengthIsLast /* 5.1 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "<ipAddress></ipAddress>\r\n"
               "<portNo>3580</portNo>\r\n"
               "<protocol>HTTP</protocol>\r\n"
               "<macAddress>98:8b:0a:f4:85:2a</macAddress>\r\n"
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-08-04T00:28:27--8:00</dateTime>\r\n"
               "<activePostCount>0</activePostCount>\r\n"
               "<eventType>HDS</eventType>\r\n"
               "<eventState>active</eventState>\r\n"
               "<eventDescription>hdStatus alarm</eventDescription>\r\n"
               "<HDList>\r\n"
               "<hd_exist>1</hd_exist>\r\n"
               "</HDList>\r\n"
               "</EventNotificationAlert>\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryEnd(), 588 );
}

TEST_F( MultipartBoundaryTests, PartWithContentLengthIsNotLast /* 5.2 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "<ipAddress></ipAddress>\r\n"
               "<portNo>3580</portNo>\r\n"
               "<protocol>HTTP</protocol>\r\n"
               "<macAddress>98:8b:0a:f4:85:2a</macAddress>\r\n"
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-08-04T00:28:27--8:00</dateTime>\r\n"
               "<activePostCount>0</activePostCount>\r\n"
               "<eventType>HDS</eventType>\r\n"
               "<eventState>active</eventState>\r\n"
               "<eventDescription>hdStatus alarm</eventDescription>\r\n"
               "<HDList>\r\n"
               "<hd_exist>1</hd_exist>\r\n"
               "</HDList>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "--hikboundary" );

    EXPECT_EQ( mbp.getLastBoundaryEnd(), 603 );
}

TEST_F( MultipartBoundaryTests, PartWithContentLengthfollowedByIncompleteHeaders /* 5.3 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "<ipAddress></ipAddress>\r\n"
               "<portNo>3580</portNo>\r\n"
               "<protocol>HTTP</protocol>\r\n"
               "<macAddress>98:8b:0a:f4:85:2a</macAddress>\r\n"
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-08-04T00:28:27--8:00</dateTime>\r\n"
               "<activePostCount>0</activePostCount>\r\n"
               "<eventType>HDS</eventType>\r\n"
               "<eventState>active</eventState>\r\n"
               "<eventDescription>hdStatus alarm</eventDescription>\r\n"
               "<HDList>\r\n"
               "<hd_exist>1</hd_exist>\r\n"
               "</HDList>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryEnd(), 588 );
}

TEST_F( MultipartBoundaryTests, PartWithContentLengthfollowedByIncompleteBody /* 5.4 */ ) {
    MultipartBoundaryProcessor mbp;
    mbp.processHeader( "Content-Type", "multipart/mixed; boundary=hikboundary" );

    pushBytes( mbp,
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n"
               "<ipAddress></ipAddress>\r\n"
               "<portNo>3580</portNo>\r\n"
               "<protocol>HTTP</protocol>\r\n"
               "<macAddress>98:8b:0a:f4:85:2a</macAddress>\r\n"
               "<channelID>1</channelID>\r\n"
               "<dateTime>2020-08-04T00:28:27--8:00</dateTime>\r\n"
               "<activePostCount>0</activePostCount>\r\n"
               "<eventType>HDS</eventType>\r\n"
               "<eventState>active</eventState>\r\n"
               "<eventDescription>hdStatus alarm</eventDescription>\r\n"
               "<HDList>\r\n"
               "<hd_exist>1</hd_exist>\r\n"
               "</HDList>\r\n"
               "</EventNotificationAlert>\r\n"
               "--hikboundary\r\n"
               "Content-Type: application/xml; charset=\"UTF-8\"\r\n"
               "Content-Length: 502\r\n"
               "\r\n"
               "<EventNotificationAlert version=\"1.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n" );

    EXPECT_EQ( mbp.getLastBoundaryEnd(), 1176 );
}

}  // namespace UT
}  // namespace HubPlatform
