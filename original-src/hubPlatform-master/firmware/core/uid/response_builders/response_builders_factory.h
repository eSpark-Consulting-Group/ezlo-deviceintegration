#pragma once

#include "jsonrpc/message.h"
#include "filter_data_list.h"

#include "api_version.h"

#include <map>
#include <memory>
#include <string>

namespace HubPlatform {

namespace CoreD {
namespace Model {

class Configuration;

}  // namespace Model
}  // namespace CoreD

namespace Uid {
namespace ResponseBuilder {
namespace GetDevices {

class IResponseBuilder;

}  // namespace GetDevices

namespace GetItems {

class IResponseBuilder;

}  // namespace GetItems

namespace DeviceSettingsGet {

class IResponseBuilder;

}  // namespace DeviceSettingsGet

namespace HubModesGet {

class IResponseBuilder;

}  // namespace HubModesGet

namespace DataListDevices {
class IResponseBuilder;
}

namespace DataListItems {
class IResponseBuilder;
}

namespace GatewaySettingsGet {
class IResponseBuilder;
}

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {

class ResponseBuildersFactory {
public:
    static std::unique_ptr< ResponseBuilder::GetDevices::IResponseBuilder > createGetDevicesResponseBuilder(
        const rpc::Message& message );

    static std::unique_ptr< ResponseBuilder::GetItems::IResponseBuilder > createGetItemsResponseBuilder(
        const rpc::Message& message,
        CoreD::Model::Configuration& configuration );

    static std::unique_ptr< ResponseBuilder::DeviceSettingsGet::IResponseBuilder >
    createGetDeviceSettingResponseBuilder( const rpc::Message& message, CoreD::Model::Configuration& configuration );

    static std::unique_ptr< ResponseBuilder::HubModesGet::IResponseBuilder > createHubModesGetBuilder(
        const rpc::Message& message );

    static std::unique_ptr< ResponseBuilder::DataListDevices::IResponseBuilder > createDataListDevicesResponseBuilder(
        const rpc::Message& rpcMessage,
        FilterDataListI*& filter );

    static std::unique_ptr< ResponseBuilder::DataListItems::IResponseBuilder > createDataListItemsResponseBuilder(
        const rpc::Message& rpcMessage,
        CoreD::Model::Configuration& configuration,
        FilterDataListI*& filter );

    static std::unique_ptr< ResponseBuilder::GatewaySettingsGet::IResponseBuilder >
    createGetGatewaySettingResponseBuilder( const rpc::Message& message, CoreD::Model::Configuration& configuration );

protected:
    template < typename ResponseBuilderI, typename ResponseBuilderT, typename... Args >
    static std::unique_ptr< ResponseBuilderI > create( const rpc::Message& message,
                                                       const std::string& api,
                                                       Args&&... args );

    template < typename ResponseBuilderI, typename... Args >
    using FactoryMethod = std::unique_ptr< ResponseBuilderI > ( * )( const rpc::Message& message,
                                                                     const std::string& api,
                                                                     Args... args );

    template < typename ResponseBuilderI, typename... Args >
    using FactoryMap = std::map< ApiVersion, FactoryMethod< ResponseBuilderI, Args... > >;

    template < typename ResponseBuilderI, typename... Args >
    static std::unique_ptr< ResponseBuilderI > createResponseBuilder(
        const FactoryMap< ResponseBuilderI, Args... >& factoryMap,
        const rpc::Message& message,
        Args&&... args );
};

}  // namespace Uid
}  // namespace HubPlatform
