#include "uid/response_builders/getitems/response_builder_v1.h"
#include "cored/model/configuration/configuration.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/message.h"
#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetItems {
namespace V1 {
namespace UT {

using namespace CoreD::Model;

class ItemsResponseBuilderTestSuite : public ::testing::Test, public Configuration, public rpc::Message {
protected:
    std::unique_ptr< ResponseBuilder > createResponseBuilder() {
        return std::make_unique< ResponseBuilder >( *this, "test", *this );
    }

    GenericSenderPtr getNetworkObject() const override {
        return {};
    }

    std::string getScaleForValueType( const ValueType& ) const override {
        return {};
    }
};

TEST_F( ItemsResponseBuilderTestSuite, deprectedDictionaryFormat ) {
    // token
    {
        auto jsonDictionaryValue = JSON_EMPTY_OBJ;
        jsonDictionaryValue[ "1" ] = "a";

        auto value = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Token ),
                                                    jsonDictionaryValue );

        auto responseBuilder = createResponseBuilder();

        IResponseBuilder::OptionalFields optionalFields;
        optionalFields.enumeration = { "a" };

        responseBuilder->addItem( "my_id", "my_dev_id", "my_name", true, false, true, *value, optionalFields );

        const auto expected = R"({
            "api": "test",
            "result": {
              "items": [
                  {
                      "_id": "my_id",
                      "deviceId": "my_dev_id",
                      "elementType": "token",
                      "enum": [ "a" ],
                      "hasGetter": true,
                      "hasSetter": false,
                      "name": "my_name",
                      "show": true,
                      "value": { "1": "a" },
                      "valueFormatted": "",
                      "valueType": "dictionary"
                  }
              ]
            }
        })";

        ASSERT_EQ( responseBuilder->buildResponse(), Json::parse( expected ) );
    }

    // user code
    {
        auto value = ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::UserCode ),
                                                    Json::parse( R"({ "1": { "code": "xyz", "name": "abc" }})" ) );

        auto responseBuilder = createResponseBuilder();

        responseBuilder->addItem( "my_id", "my_dev_id", "my_name", true, true, true, *value, {} );

        const auto expected = R"({
          "api": "test",
          "result": {
              "items": [
                  {
                      "_id": "my_id",
                      "deviceId": "my_dev_id",
                      "elementType": "userCode",
                      "hasGetter": true,
                      "hasSetter": true,
                      "name": "my_name",
                      "show": true,
                      "value": { "1": { "code": "xyz", "name": "abc" } },
                      "valueFormatted": "",
                      "valueType": "dictionary"
                  }
              ]
          }
        })";

        ASSERT_EQ( responseBuilder->buildResponse(), Json::parse( expected ) );
    }
}

TEST_F( ItemsResponseBuilderTestSuite, dictionaryFormat ) {
    // int
    {
        auto jsonDictionaryValue = JSON_EMPTY_OBJ;
        jsonDictionaryValue[ "1" ] = 1;

        auto value =
            ValueFactory::createItemValue( ValueType::makeDictionaryType( ValueType::Enum::Int ), jsonDictionaryValue );

        auto responseBuilder = createResponseBuilder();

        responseBuilder->addItem( "my_id", "my_dev_id", "my_name", true, false, true, *value, {} );

        const auto expected = R"({
            "api": "test",
            "result": {
              "items": [
                  {
                      "_id": "my_id",
                      "deviceId": "my_dev_id",
                      "hasGetter": true,
                      "hasSetter": false,
                      "name": "my_name",
                      "show": true,
                      "value": { "1": 1 },
                      "valueFormatted": "",
                      "valueType": "dictionary",
                      "elementType": "int"
                  }
              ]
            }
        })";

        ASSERT_EQ( responseBuilder->buildResponse(), Json::parse( expected ) );
    }

    // user code
    {
        auto value = ValueFactory::createItemValue(
            ValueType::makeDictionaryType( ValueType::makeArrayType( ValueType::Enum::String ) ),
            Json::parse( R"({ "1": [ "str1", "str2" ] })" ) );

        auto responseBuilder = createResponseBuilder();

        IResponseBuilder::OptionalFields optionalFields;
        optionalFields.elementsMaxNumbers = { { 0, 10 }, { 1, 20 } };

        responseBuilder->addItem( "id", "dev_id", "name", true, true, true, *value, optionalFields );

        const auto expected = R"({
          "api": "test",
          "result": {
              "items": [
                  {
                      "_id": "id",
                      "deviceId": "dev_id",
                      "hasGetter": true,
                      "hasSetter": true,
                      "name": "name",
                      "show": true,
                      "value": { "1": [ "str1", "str2" ] },
                      "valueFormatted": "",
                      "valueType": "dictionary",
                      "elementType": "array.string",
                      "elementsMaxNumber": 10,
                      "elementsMaxNumberPerArray": 20
                  }
              ]
          }
        })";

        ASSERT_EQ( responseBuilder->buildResponse(), Json::parse( expected ) );
    }
}

}  // namespace UT
}  // namespace V1
}  // namespace GetItems
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
