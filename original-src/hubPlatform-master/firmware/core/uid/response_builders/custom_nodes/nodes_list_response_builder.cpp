#include "nodes_list_response_builder.h"
#include "service_interfaces/service.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "extensions_manager/plugin_model/model_resources.h"
#include "database_client/dbqueryparser.h"
#include "uid/ui_service.h"
#include "system_utils/environment.h"

using namespace ::ExtensionsManager::Resources::ConfigFields;
namespace HubPlatform {
namespace Uid {
namespace CustomNodes {

NodesListResponseBuilder::NodesListResponseBuilder( Services::Service& service )
: m_service( service ) {}

void NodesListResponseBuilder::setAuthorFilteringParam( const std::string& author ) {
    m_author = author;
}

void NodesListResponseBuilder::setPluginName( const std::string& pluginName ) {
    m_pluginName = pluginName;
}

void NodesListResponseBuilder::setPluginShortName( const std::string& shortName ) {
    m_pluginShortName = shortName;
}

void NodesListResponseBuilder::setShowMeta( bool showMeta ) {
    m_showMeta = showMeta;
}

void NodesListResponseBuilder::setShowAdvanced( bool showAdvanced ) {
    m_showAdvanced = showAdvanced;
}

void NodesListResponseBuilder::setShowInterface( bool showInterface ) {
    m_showInterface = showInterface;
}

void NodesListResponseBuilder::setIds( std::set< std::string > ids ) {
    m_ids = std::move( ids );
}

JsonObject NodesListResponseBuilder::extractMetadataFromItem( const JsonObject& item ) {
    auto res = JSON( Meta::Name << item[ DbFields::MetaName ] << Meta::Description << item[ DbFields::MetaDescription ]
                                << Meta::Author << item[ DbFields::MetaAuthor ] << Meta::Prefix
                                << item[ DbFields::MetaPrefix ] << Meta::Type << item[ DbFields::MetaType ]
                                << Meta::Language << item[ DbFields::MetaLanguage ] << Meta::Size
                                << item[ DbFields::MetaSize ] << Meta::Placement << item[ DbFields::Placement ] );
    return res;
}

JsonObject NodesListResponseBuilder::extractConfigurationFromItem( const JsonObject& item ) {
    JsonObject res;
    json_get( item, Root::Configuration, res );
    return res;
}

JsonObject NodesListResponseBuilder::getDataWithFilter() {
    LOGGER_TRACE_RAW( cc::str( "getDataWithFilter m_showInterface:" + cc::flag( m_showInterface ) ) );
    JsonObject query;
    auto extensionManager = std::make_unique< ExtensionsManager::RemoteInterface >( m_service );

    if( !m_pluginName.empty() ) {
        query[ DbFields::Plugin ] = m_pluginName;
    }
    if( !m_pluginShortName.empty() ) {
        query[ DbFields::ShortName ] = m_pluginShortName;
    }
    if( !m_author.empty() ) {
        query[ DbFields::MetaAuthor ] = m_author;
    }

    auto items = extensionManager->fetchPluginsMetadataWithQuery( query );

    // deserialize config.json to place to db
    JsonArrayBuilder nodes;
    for( auto dbItem : items ) {
        JsonObject node;

        auto id = json_get_str( dbItem, db::ID_FIELD_NAME );
        if( m_ids.count( id ) != 0 || m_ids.empty() ) {
            node[ db::ID_FIELD_NAME ] = dbItem[ db::ID_FIELD_NAME ];
            node[ DbFields::Plugin ] = dbItem[ DbFields::Plugin ];
            node[ DbFields::ShortName ] = dbItem[ DbFields::ShortName ];
            node[ Root::Version ] = dbItem[ DbFields::Version ];

            if( m_showMeta ) {
                node[ Root::Meta ] = extractMetadataFromItem( dbItem );
            }
            if( m_showAdvanced ) {
                node[ Root::Configuration ] = extractConfigurationFromItem( dbItem );
            }
            if( m_showInterface ) {
                node[ Root::Interface ] = extractInterfaceContent( dbItem[ DbFields::Plugin ] );
            }
            nodes << node;
        }
    }
    auto res = JSON( "nodes" << nodes );
    return res;
}

JsonObject NodesListResponseBuilder::extractInterfaceContent( const std::string& nodeName ) {
    auto pluginDir = HubPlatform::Environment::getCustomPluginsFolder() + "/" + nodeName;
    std::string interfaceFilePath = pluginDir + "/" + ::ExtensionsManager::Resources::PluginFiles::InterfaceFileName;
    std::ifstream interfaceFile( interfaceFilePath );
    std::stringstream ss;
    if( interfaceFile.is_open() ) {
        ss << interfaceFile.rdbuf();
    }
    JsonObject obj;
    try {
        obj[ "contents" ] = Json::parse( ss );
    }
    catch( const std::exception& e ) {
        obj[ "error" ] = Errors::get( Errors::JSON_PARSE_ERROR );
    }

    return obj;
}

}  // namespace CustomNodes
}  // namespace Uid
}  // namespace HubPlatform
