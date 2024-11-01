#pragma once

#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

class ResponseBuilderBase {
public:
    ResponseBuilderBase( const rpc::Message& message, const std::string& api );

    virtual ~ResponseBuilderBase() = default;

    virtual JsonObject buildResponse() const;

private:
    const rpc::Message& m_message;
    const std::string m_api;
};

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
