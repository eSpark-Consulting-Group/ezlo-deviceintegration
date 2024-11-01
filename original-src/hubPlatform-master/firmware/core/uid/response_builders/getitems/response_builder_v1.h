#pragma once

#include "iresponse_builder.h"

#include "uid/response_builders/response_builder_base.h"
#include "uid/response_builders/filter_data_list.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Configuration;

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace GetItems {
namespace V1 {

class ResponseBuilder : public ResponseBuilderBase, public IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message,
                     const std::string& api,
                     CoreD::Model::Configuration& configuration,
                     FilterDataListI* filter = nullptr );

    void addItem( const std::string& id,
                  const std::string& deviceId,
                  const std::string& name,
                  bool hasGetter,
                  bool hasSetter,
                  bool show,
                  const CoreD::Model::Value& value,
                  const OptionalFields& optionalFields ) override;

    JsonObject buildResponse() const override;

protected:
    void writeOptionalField( JsonObject& response,
                             CoreD::Model::Value& value,
                             const std::string& itemField,
                             const std::string& responseField );

protected:
    HubPlatform::CoreD::Model::Configuration& m_configuration;
    std::vector< JsonObject > m_items;
    FilterDataListI* m_filter;
};

}  // namespace V1
}  // namespace GetItems

namespace DataListItems {
namespace V1 {

class ResponseBuilder : public GetItems::V1::ResponseBuilder, public DataListItems::IResponseBuilder {
public:
    ResponseBuilder( const rpc::Message& message,
                     const std::string& api,
                     CoreD::Model::Configuration& configuration,
                     FilterDataListI* filter = nullptr );
    virtual ~ResponseBuilder() = default;

    void addItem( const std::string& id,
                  const std::string& deviceId,
                  const std::string& name,
                  bool hasGetter,
                  bool hasSetter,
                  bool show,
                  const CoreD::Model::Value& value,
                  const OptionalFields& optionalFields ) override;

    JsonObject buildResponse() const override;
};

}  // namespace V1
}  // namespace DataListItems

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
