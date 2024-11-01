
#include "uid/response_builders/response_builders_factory.h"

#include "cored/model/configuration/configuration.h"

#include "uid/response_builders/getdevices/response_builder_v1.h"
#include "uid/response_builders/getitems/response_builder_v1.h"
#include "uid/response_builders/device_settings_get/response_builder_v1.h"
#include "uid/response_builders/hub_modes_get/response_builder_v2.h"
#include "uid/response_builders/gateway_settings_get/response_builder_v1.h"
#include "uid/response_builders/hub_modes_get/response_builder_v3.h"

#include "common_utils/cpp14.h"

#include <memory>

namespace HubPlatform {
namespace Uid {

std::unique_ptr< ResponseBuilder::GetDevices::IResponseBuilder >
ResponseBuildersFactory::createGetDevicesResponseBuilder( const rpc::Message& message ) {
    using namespace ResponseBuilder::GetDevices;

    static const FactoryMap< IResponseBuilder > factoryMap = { { { 1, 0 },
                                                                 create< IResponseBuilder, V1::ResponseBuilder > } };

    return createResponseBuilder( factoryMap, message );
}

std::unique_ptr< ResponseBuilder::GetItems::IResponseBuilder > ResponseBuildersFactory::createGetItemsResponseBuilder(
    const rpc::Message& message,
    CoreD::Model::Configuration& configuration ) {
    using namespace ResponseBuilder::GetItems;

    using Configuration = CoreD::Model::Configuration;
    static const FactoryMap< IResponseBuilder, Configuration& > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder, Configuration& > }
    };

    return createResponseBuilder( factoryMap, message, configuration );
}

std::unique_ptr< ResponseBuilder::DeviceSettingsGet::IResponseBuilder >
ResponseBuildersFactory::createGetDeviceSettingResponseBuilder( const rpc::Message& message,
                                                                CoreD::Model::Configuration& configuration ) {
    using namespace ResponseBuilder::DeviceSettingsGet;

    using Configuration = CoreD::Model::Configuration;
    static const FactoryMap< IResponseBuilder, Configuration& > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder, Configuration& > }
    };

    return createResponseBuilder( factoryMap, message, configuration );
}

std::unique_ptr< ResponseBuilder::DataListDevices::IResponseBuilder >
ResponseBuildersFactory::createDataListDevicesResponseBuilder( const rpc::Message& message, FilterDataListI*& filter ) {
    using namespace ResponseBuilder::DataListDevices;

    static const FactoryMap< IResponseBuilder, FilterDataListI*& > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder, FilterDataListI*& > }
    };

    return createResponseBuilder( factoryMap, message, filter );
}

std::unique_ptr< ResponseBuilder::DataListItems::IResponseBuilder >
ResponseBuildersFactory::createDataListItemsResponseBuilder( const rpc::Message& message,
                                                             CoreD::Model::Configuration& configuration,
                                                             FilterDataListI*& filter ) {
    using namespace ResponseBuilder::DataListItems;
    using Configuration = CoreD::Model::Configuration;

    static const FactoryMap< IResponseBuilder, Configuration&, FilterDataListI*& > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder, Configuration&, FilterDataListI*& > }
    };

    return createResponseBuilder( factoryMap, message, configuration, filter );
}

std::unique_ptr< ResponseBuilder::GatewaySettingsGet::IResponseBuilder >
ResponseBuildersFactory::createGetGatewaySettingResponseBuilder( const rpc::Message& message,
                                                                 CoreD::Model::Configuration& configuration ) {
    using namespace ResponseBuilder::GatewaySettingsGet;

    using Configuration = CoreD::Model::Configuration;
    static const FactoryMap< IResponseBuilder, Configuration& > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder, Configuration& > }
    };

    return createResponseBuilder( factoryMap, message, configuration );
}

std::unique_ptr< ResponseBuilder::HubModesGet::IResponseBuilder > ResponseBuildersFactory::createHubModesGetBuilder(
    const rpc::Message& message ) {
    using namespace ResponseBuilder::HubModesGet;

    static const FactoryMap< IResponseBuilder > factoryMap = {
        { { 1, 0 }, create< IResponseBuilder, V1::ResponseBuilder > },
        { { 2, 0 }, create< IResponseBuilder, V2::ResponseBuilder > },
        { { 3, 0 }, create< IResponseBuilder, V3::ResponseBuilder > }
    };

    return createResponseBuilder( factoryMap, message );
}

template < typename ResponseBuilderI, typename... Args >
std::unique_ptr< ResponseBuilderI > ResponseBuildersFactory::createResponseBuilder(
    const FactoryMap< ResponseBuilderI, Args... >& factoryMap,
    const rpc::Message& message,
    Args&&... args ) {
    if( factoryMap.empty() )
        return nullptr;

    auto it = factoryMap.end();

    const ApiVersion apiVersion( json_get_str( message.getMessage(), "api" ) );
    if( apiVersion.isValid() ) {
        it = factoryMap.find( apiVersion );
        if( it == factoryMap.end() ) {
            auto lowestApiIt = factoryMap.cbegin();
            if( apiVersion < lowestApiIt->first ) {
                it = lowestApiIt;
            }
            else {
                it = std::prev( factoryMap.end() );
            }
        }
    }
    else {
        it = factoryMap.cbegin();
    }

    const auto& factoryMethod = it->second;
    return factoryMethod( message, it->first.toString(), std::forward< Args >( args )... );
}

template < typename ResponseBuilderI, typename ResponseBuilderT, typename... Args >
std::unique_ptr< ResponseBuilderI > ResponseBuildersFactory::create( const rpc::Message& message,
                                                                     const std::string& api,
                                                                     Args&&... args ) {
    return std::make_unique< ResponseBuilderT >( message, api, std::forward< Args >( args )... );
}

}  // namespace Uid
}  // namespace HubPlatform
