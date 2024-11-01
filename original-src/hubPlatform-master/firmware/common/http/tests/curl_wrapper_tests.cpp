// Tests for curl_wrapper from common_utils
#include "common_utils/curl_wrapper.h"
#include "json_utils/json_utils.h"
#include "system_utils/find_free_local_port.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class CurlWrapperTests : public ::testing::Test {};

/** Calls to external service, only for manual testing for now*/
TEST_F( CurlWrapperTests, DISABLED_requestGetOnSpecificInterface ) {
    std::string interface = "wlp2s0";  // call on this interface
    std::string url = "https://cloud.ezlo.com:7000";
    std::string httpResponse;

    const int RequestFailed = 699;
    long httpStatusCode = RequestFailed;
    std::chrono::seconds transferTimeout( 10 );

    if( transferTimeout.count() == 0 )
        transferTimeout = std::chrono::seconds( 10 );
    LOGGER_INFO( cc::info( "Test internet with HTTP request: " ) + url + ", interface: " + interface );

    auto connectTimeout = 10;
    // Make insecure HTTP(s) request on a specific interface
    int statusOrError = CurlWrapper::curlCustomGet(
        url, {}, "", "", "", httpResponse, connectTimeout, transferTimeout.count(), interface.c_str() );

    if( statusOrError == 0 || statusOrError == RequestFailed ) {
        LOGGER_ERROR( cc::error( "HTTP Check failed: http status=" ) + cc::num10( (int64_t) httpStatusCode ) +
                      ", url: " + url + ", interface: " + interface + ", last status code=" +
                      cc::num10( CurlWrapper::lastStatusCode() ) + ", text: " + CurlWrapper::lastStatusCodeText() );
    }
    else {
        LOGGER_DEBUG( cc::debug( "HTTP Check successful, HTTP status: " ) + cc::num10( statusOrError ) +
                      ", url: " + url + ", interface: " + interface );
    }

    ASSERT_EQ( statusOrError, 0 ) << "Url not found, return 0";
    ASSERT_EQ( CurlWrapper::lastStatusCode(), 404 ) << "Url not found, http status is 404";
}

}  // namespace UT
}  // namespace HubPlatform
