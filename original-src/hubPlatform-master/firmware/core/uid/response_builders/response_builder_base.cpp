#include "response_builder_base.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

ResponseBuilderBase::ResponseBuilderBase( const rpc::Message& message, const std::string& api )
: m_message( message )
, m_api( api ) {}

JsonObject ResponseBuilderBase::buildResponse() const {
    JsonObject joResponse = JSON_EMPTY_OBJ;
    joResponse[ "api" ] = m_api;

    return joResponse;
}

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
