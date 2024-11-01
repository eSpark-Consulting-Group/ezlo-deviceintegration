#pragma once

#include "iresponse_builder.h"

#include "uid/response_builders/response_builder_base.h"
#include "uid/response_builders/filter_data_list.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

class DevicesListBuilder {
public:
    DevicesListBuilder();

    static void buildDataListDevicesResponse( rpc::Message& rpcMessage,
                                              const std::vector< JsonObject >& devices,
                                              FilterDataListI* filter );
};

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
