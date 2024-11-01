#include "custom_nodes_list.h"
#include "extensions_manager/plugin_model/model_resources.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "uid/response_builders/custom_nodes/nodes_list_response_builder.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {

CustomNodesList::CustomNodesList( Services::Service& service )
: m_service( service ) {}

void CustomNodesList::setWrongParamError( rpc::Message& rpcMessage, const std::string& key ) {
    JsonObject error;
    Errors::getWrongParams( key, error );
    setErrorObject( rpcMessage.getAnswer(), error );
}

void CustomNodesList::handler( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    using namespace ::ExtensionsManager::Resources::ConfigFields;

    JsonArray idsVector;
    std::set< std::string > ids;

    constexpr auto Ids = "ids";
    if( params.find( Ids ) != params.cend() ) {
        if( params[ Ids ].is_array() ) {
            json_get_arr( params, Ids, idsVector );
            for( const auto& id : idsVector ) {
                ids.insert( json_get_str( id ) );
            }
        }
        else {
            JsonObject error;
            Errors::getWrongParams( Ids, error );
            setErrorObject( rpcMessage.getAnswer(), error );
            setWrongParamError( rpcMessage, Ids );
            return;
        }
    }

    std::string filterId = json_get_str( params, "id" );
    if( !filterId.empty() ) {
        ids.insert( filterId );
    }

    std::string name;

    if( params.find( Meta::Name ) != params.cend() ) {
        if( params[ Meta::Name ].is_string() ) {
            name = json_get_str( params, Meta::Name );
        }
        else {
            setWrongParamError( rpcMessage, Meta::Name );
            return;
        }
    }
    std::string shortName = json_get_str( params, DbFields::ShortName );

    std::string author;
    if( params.find( Meta::Author ) != params.cend() ) {
        if( params[ Meta::Author ].is_string() ) {
            author = json_get_str( params, Meta::Author );
        }
        else {
            setWrongParamError( rpcMessage, Meta::Author );
            return;
        }
    }

    bool showMeta;
    constexpr auto showMetaKey = "showMeta";
    if( params.find( showMetaKey ) != params.cend() ) {
        if( params[ showMetaKey ].is_boolean() ) {
            showMeta = json_get_bool( params, showMetaKey );
        }
        else {
            setWrongParamError( rpcMessage, showMetaKey );
            return;
        }
    }

    bool showAdvanced;
    constexpr auto showAdvancedKey = "showAdvanced";
    if( params.find( showAdvancedKey ) != params.cend() ) {
        if( params[ showAdvancedKey ].is_boolean() ) {
            showAdvanced = json_get_bool( params, showAdvancedKey );
        }
        else {
            setWrongParamError( rpcMessage, showAdvancedKey );
            return;
        }
    }

    bool showInterface;
    constexpr auto showInterfaceKey = "showInterface";
    if( params.find( showInterfaceKey ) != params.cend() ) {
        if( params[ showInterfaceKey ].is_boolean() ) {
            showInterface = json_get_bool( params, showInterfaceKey );
        }
        else {
            setWrongParamError( rpcMessage, showInterfaceKey );
            return;
        }
    }

    CustomNodes::NodesListResponseBuilder responseBuilder( m_service );
    responseBuilder.setPluginName( name );
    responseBuilder.setPluginShortName( shortName );
    responseBuilder.setAuthorFilteringParam( author );
    responseBuilder.setIds( ids );
    responseBuilder.setShowMeta( showMeta );
    responseBuilder.setShowAdvanced( showAdvanced );
    responseBuilder.setShowInterface( showInterface );
    auto& answer = rpcMessage.getAnswer();

    try {
        answer[ "result" ] = responseBuilder.getDataWithFilter();
    }
    catch( const std::exception& e ) {
        ::setError( rpcMessage.getAnswer(), Errors::INTERNAL_OPERATION_ERROR, e.what() );
    }
}
}  // namespace Uid
}  // namespace HubPlatform