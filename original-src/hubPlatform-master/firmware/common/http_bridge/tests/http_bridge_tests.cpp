#include "http_bridge/command.h"
#include "http_bridge/errors.h"
#include <gtest/gtest.h>
#include "json_utils/json_utils.h"
#include <limits>
#include <ios>

namespace HubPlatform {
namespace UT {

using namespace HubPlatform::HttpBridge;

class HttpBridgeUrlParserTests : public ::testing::Test {};

/**
 *  TESTPLAN
 *  1 Requests
 *      1.1 Parse an URL from GET request and split it to components.
 *      1.2 Parse an URL from GET, throw errors.
 */

TEST_F( HttpBridgeUrlParserTests, parseMethodFromGetUrl ) /* 1.1 */ {
    static std::pair< std::string, Json > urls[] = {
        { "/v1/method/hub/info/get", R"({"api":"1.0","method":"hub.info.get","params":{}})"_json },
        { "/v1/method/hub/info/get?_id=111dddaaa444",
          R"({"api":"1.0","method":"hub.info.get","params":{"_id":"111dddaaa444"}})"_json },
        { "/v1/method/hub/info/get?_id=111dddaaa444"
          "&test=typed_parameters&silent_bool=true&int_number_int=3&float_number_float=3.141590#4_5",
          R"({"api":"4.5", "method":"hub.info.get", "params":{"_id":"111dddaaa444","float_number":3.1415901184082,"int_number":3,
                 "silent":true,"test":"typed_parameters"}})"_json },
        { "/v1/method/hub/item/value/set",
          R"({"api":"1.0","method":"hub.item.value.set","params":{}})"_json }
    };

    for( const auto& pair : urls ) {
        HttpBridgeCommand command;

        const auto& url = pair.first;
        ASSERT_TRUE( command.parse( url ) ) << "Url: " << url;
        Json json;
        ASSERT_NO_THROW( json = command.toJson() ) << url;
        LOGGER_DEBUG_RAW( cc::debug( "Method parsed: " ) + command.method() + cc::debug( ", json dump: " ) +
                          cc::j( json ) + cc::debug( ", Source: " ) + url );
        EXPECT_EQ( json.dump(), pair.second.dump() ) << url;
    }
}

TEST_F( HttpBridgeUrlParserTests, parseMethodFromGetUrlAndThrow ) /* 1.2 */ {
    auto makeOutOfRangeDouble = []() -> std::string {
        std::stringstream ss;
        ss << std::scientific << std::numeric_limits< double >::max();

        auto str = "1000" + ss.str();
        return str;
    };

    std::pair< std::string, Json > urls[] = {
        { "/v1/method/hub/info/get?value_bool=true111", R"({})"_json },
        { "/v1/method/hub/info/get?value_bool=111", R"({})"_json },
        { "/v1/method/hub/info/get?value_int=111zzz", R"({})"_json },
        { "/v1/method/hub/info/get?value_int=111111111111111111111111111111111111111111", R"({})"_json },
        { "/v1/method/hub/info/get?value_float=string", R"({})"_json },
        { "/v1/method/hub/info/get?value_float=3.14zzz", R"({})"_json },
        { std::string{ "/v1/method/hub/info/get?value_float=" } + makeOutOfRangeDouble(), R"({})"_json }
    };

    for( const auto& pair : urls ) {
        HttpBridgeCommand command;

        const auto& url = pair.first;
        try {
            ASSERT_TRUE( command.parse( url ) ) << "Url: " << url;
            Json json = command.toJson();
            LOGGER_DEBUG_RAW( cc::debug( "Method parsed: " ) + command.method() + cc::debug( ", json dump: " ) +
                              cc::j( json ) + cc::debug( ", Source: " ) + url );
            EXPECT_EQ( json.dump(), pair.second.dump() ) << url;
        }
        catch( HubPlatform::HttpBridge::ApiBridgeError& ex ) {
            LOGGER_DEBUG_RAW( std::string{ "Exception is caught: " } + ex.what() );
            SUCCEED() << "Command parsing should throw an error, error: '" << ex.what() << "' for an url '" << url
                      << "'";
        }
        catch( std::exception& ex ) {
            FAIL() << "Command parsing has failed, error: " << ex.what();
        }
    }
}

}  // namespace UT
}  // namespace HubPlatform
