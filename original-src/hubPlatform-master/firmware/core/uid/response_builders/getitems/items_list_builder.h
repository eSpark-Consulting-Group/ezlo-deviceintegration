#pragma once

#include "iresponse_builder.h"

#include "uid/response_builders/response_builder_base.h"
#include "uid/response_builders/filter_data_list.h"
#include "service_interfaces/service.h"
#include "cored/model/values/value.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

class Service;

using OptionalFieldsUPtr = std::unique_ptr< Uid::ResponseBuilder::GetItems::IResponseBuilder::OptionalFields >;
using PreparedItemFields = std::tuple< CoreD::Model::ValueUPtr, OptionalFieldsUPtr >;

class ItemsListBuilder {
public:
    ItemsListBuilder();

    static void buildGetItemsResponse( rpc::Message& rpcMessage,
                                       const std::vector< JsonObject >& items,
                                       Services::Service& service );

    static PreparedItemFields prepareItemFields( const JsonObject& item );
};

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
