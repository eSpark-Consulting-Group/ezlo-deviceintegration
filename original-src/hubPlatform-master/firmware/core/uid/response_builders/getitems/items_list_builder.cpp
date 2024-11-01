#include "items_list_builder.h"
#include "uid/response_builders/response_builders_factory.h"
#include "response_builder_v1.h"
#include "uid/response_builders/filter_data_list.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "uid/response_builders/hub_object_list_device_filter.h"
#include "service_interfaces/service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value.h"
#include "cored/model/exceptions.h"
#include "cored/resources/core_resources.h"
#include "service_interfaces/bus_wrapper.h"
#include "json_utils/json_custom_getters.h"
#include "jsonrpc/error_codes.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

PreparedItemFields ItemsListBuilder::prepareItemFields( const JsonObject& item ) {
    using namespace ResponseBuilder::GetItems;
    using namespace CoreD::Model;
    using namespace JsonCustomGetters;
    using namespace Resources;

    //    IResponseBuilder::OptionalFields optionalFields;
    OptionalFieldsUPtr optionalFieldsPtr = std::make_unique< IResponseBuilder::OptionalFields >();
    if( !optionalFieldsPtr ) {
        throw std::runtime_error( "Could not create optional fields" );
    }

    auto& optionalFields = *optionalFieldsPtr.get();

    const auto valueType = ValueType::fromString( json_get_str( item, ItemFields::TypeFieldName ) );

    auto value = ValueFactory::createItemValue( valueType, item[ ItemFields::ValueFieldName ] );

    if( item.count( ItemFields::ValueMinFieldName ) > 0 ) {
        optionalFields.valueMin = ValueFactory::createItemValue( valueType, item[ ItemFields::ValueMinFieldName ] );
    }

    if( item.count( ItemFields::ValueMaxFieldName ) > 0 ) {
        optionalFields.valueMax = ValueFactory::createItemValue( valueType, item[ ItemFields::ValueMaxFieldName ] );
    }

    if( item.count( ItemFields::ValueIndentFieldName ) > 0 ) {
        optionalFields.valueIndent = item[ ItemFields::ValueIndentFieldName ];
    }

    if( item.count( ItemFields::InfoFieldName ) > 0 ) {
        optionalFields.info = item[ ItemFields::InfoFieldName ];
    }
    makeFieldGetter( ItemFields::EnumFieldName, makeDefaultGetter< Enum >() )( item, optionalFields.enumeration );
    makeFieldGetter( ItemFields::UserCodeModesFieldName, makeDefaultGetter< Enum >() )( item,
                                                                                        optionalFields.userCodeModes );
    makeFieldGetter( ItemFields::UserCodeRestrictionFieldName, makeDefaultGetter< std::string >() )(
        item, optionalFields.userCodeRestriction );
    makeFieldGetter( ItemFields::StringRestrictionFieldName, makeDefaultGetter< std::string >() )(
        item, optionalFields.stringRestriction );

    Item::ContainerLevel containerLevel = 0;
    for( auto type = &valueType; type && type->isContainer(); type = type->getSubType().get(), ++containerLevel ) {
        auto elementsCountIt = item.find( Item::getElementsMaxNumberFieldName( containerLevel ) );
        if( elementsCountIt != item.end() && elementsCountIt.value().is_number() ) {
            optionalFields.elementsMaxNumbers.emplace( containerLevel, elementsCountIt.value().get< size_t >() );
        }
    }

    return PreparedItemFields( std::move( value ), std::move( optionalFieldsPtr ) );
}

void ItemsListBuilder::buildGetItemsResponse( rpc::Message& rpcMessage,
                                              const std::vector< JsonObject >& items,
                                              Services::Service& service ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( service );

    auto responseBuilder =
        ResponseBuildersFactory::createGetItemsResponseBuilder( rpcMessage, coreDInterface.getConfiguration() );
    if( !responseBuilder )
        return;

    HubObjectListDeviceFilter deviceFilter( rpcMessage );

    try {
        for( const auto& i : items ) {
            using namespace ResponseBuilder::GetItems;
            using namespace CoreD::Model;
            using namespace JsonCustomGetters;
            using namespace Resources;

            // [EFWR-1665] filter records inserted to the report by message.params.deviceId: string[] array field
            bool isGood = deviceFilter.filterDeviceIds( json_get_str( i, "deviceId" ) );
            if( !isGood ) {
                continue;
            }

            ValueUPtr value;
            OptionalFieldsUPtr optionalFieldsPtr;
            std::tie( value, optionalFieldsPtr ) = ResponseBuilder::ItemsListBuilder::prepareItemFields( i );
            auto& optionalFields = *optionalFieldsPtr.get();

            responseBuilder->addItem( json_get_str( i, "_id" ),
                                      json_get_str( i, "deviceId" ),
                                      json_get_str( i, "name" ),
                                      json_get_bool( i, "hasGetter" ),
                                      json_get_bool( i, "hasSetter" ),
                                      json_get_bool( i, "show" ),
                                      *value,
                                      optionalFields );
        }

        JsonObject& joAnswer = rpcMessage.getAnswer();
        auto response = responseBuilder->buildResponse();
        joAnswer[ "result" ] = std::move( response[ "result" ] );
        joAnswer[ "api" ] = std::move( response[ "api" ] );
    }
    catch( const MaxMessageLengthError& e ) {
        setError( rpcMessage.getAnswer(), Errors::BUS_MESSAGE_TOO_LONG, e.what() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        Log::error( cc::error( "buildGetItemsResponse: CoreDModelError occurred - " ) + error.what() );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
    catch( ... ) {
        Log::error( cc::error( "buildGetItemsResponse: unknown error occurred - " ) );
        rpcMessage.setError( Errors::INTERNAL_OPERATION_ERROR );
    }
}

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
