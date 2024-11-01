#include "extensions_manager/remote_interface/plugin_extension_data_extractor.h"
#include "extensions_manager/resources/em_resources.h"

#include "plugin_utils/plugin_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace ExtensionsManager {

PluginExtensionDataExtractor::PluginExtensionDataExtractor( IDbClient& dbClient, PluginUtils& pluginUtils )
: m_dbClient( dbClient )
, m_pluginUtils( pluginUtils ) {}

Json PluginExtensionDataExtractor::extractData( const std::string& pluginId,
                                                const std::string& path,
                                                const std::string& filter ) {
    try {
        auto splittedPath = split( path );
        if( splittedPath.size() < 2 ) {
            return getDefaultResult( filter );
        }

        const std::string& extensionPluginId = splittedPath.at( 0 );
        const std::string& extensionName = splittedPath.at( 1 );

        std::string pathToExtensionFile = getExtensionFilePath( extensionPluginId, pluginId, extensionName );
        if( pathToExtensionFile.empty() ) {
            return getDefaultResult( filter );
        }

        auto extensionsStream = m_pluginUtils.getInternalPluginFile( extensionPluginId, pathToExtensionFile );
        if( !extensionsStream ) {
            return getDefaultResult( filter );
        }

        Json extensions;
        ( *extensionsStream ) >> extensions;

        Json& currentField = extensions;
        bool found = true;
        for( int i = 2; i < splittedPath.size(); ++i ) {
            if( currentField.is_object() && currentField.count( splittedPath.at( i ) ) ) {
                currentField = currentField[ splittedPath.at( i ) ];
            }
            else {
                found = false;
            }
        }

        if( !found ) {
            return getDefaultResult( filter );
        }

        return this->filter( currentField, filter );
    }
    catch( const std::exception& e ) {
        return getDefaultResult( filter );
    }
}

std::vector< std::string > PluginExtensionDataExtractor::split( const std::string& path, char delimeter ) const {
    std::vector< std::string > result;

    auto start = 0U;
    auto end = path.find( delimeter );
    while( end != std::string::npos ) {
        result.push_back( path.substr( start, end - start ) );
        start = end + 1;
        end = path.find( delimeter, start );
    }

    result.push_back( path.substr( start, end ) );

    return result;
}

std::string PluginExtensionDataExtractor::getExtensionFilePath( const std::string& extensionPluginId,
                                                                const std::string& extendedPluginId,
                                                                const std::string& extensionName ) {
    using namespace Resources::PluginRecord;

    const auto extensionPlugin =
        m_dbClient.findOne( Resources::DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << extensionPluginId ), nullptr );

    if( !extensionPlugin.empty() && extensionPlugin.count( Resources::PluginRecord::Extensions ) ) {
        const auto& extensions = extensionPlugin[ Resources::PluginRecord::Extensions ];
        if( extensions.count( ExtensionsRecord::Plugins ) ) {
            for( const auto& extension : extensions[ ExtensionsRecord::Plugins ] ) {
                if( extension[ ExtensionsRecord::Id ] == extendedPluginId ) {
                    for( const auto& extensionConfig : extension[ ExtensionsRecord::Configs ] ) {
                        if( extensionConfig[ ExtensionsRecord::Name ] == extensionName ) {
                            return extensionConfig[ ExtensionsRecord::Config ];
                        }
                    }
                }
            }
        }
    }

    return "";
}

Json PluginExtensionDataExtractor::filter( const Json& field, const std::string& filter ) {
    if( filter == "keys" ) {
        if( field.is_object() ) {
            JsonArray result;
            for( auto it = field.begin(); it != field.end(); ++it ) {
                result.push_back( it.key() );
            }
            return result;
        }
        else {
            return JSON_EMPTY_ARR;
        }
    }
    else if( filter == "count" ) {
        if( field.is_array() ) {
            return field.size();
        }
        else {
            return 0;
        }
    }
    else if( isNumber( filter ) ) {
        int index = std::stoi( filter );
        if( field.is_array() && index < field.size() ) {
            return field.at( index );
        }
        else {
            return JSON_EMPTY_OBJ;
        }
    }
    else {
        return field;
    }
}

Json PluginExtensionDataExtractor::getDefaultResult( const std::string& filter ) const {
    if( filter == "keys" ) {
        return JSON_EMPTY_ARR;
    }
    else if( filter == "count" ) {
        return 0;
    }
    else {
        return JSON_EMPTY_OBJ;
    }
}

bool PluginExtensionDataExtractor::isNumber( const std::string& s ) {
    return !s.empty() && std::all_of( s.begin(), s.end(), ::isdigit );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
