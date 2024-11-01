#include "plugin_utils/plugin_utils.h"

#include "system_utils/environment.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <cstring>
#include <dirent.h>
#include <tuple>

namespace HubPlatform {
namespace ExtensionsManager {

const char* PluginUtils::m_pluginScriptsPrefix = "HUB:";

PluginUtils::PluginsIds PluginUtils::getInstalledPlugins() const {
    return getPluginsFrom( Environment::getPluginsFolder() );
}

PluginUtils::PluginsIds PluginUtils::getCustomPlugins() const {
    return getPluginsFrom( Environment::getCustomPluginsFolder() );
}

PluginUtils::PluginsIds PluginUtils::getPluginsFrom( const std::string& pluginsFolderPath ) const {
    PluginsIds result;
    DIR* dir = ::opendir( pluginsFolderPath.c_str() );
    if( dir == nullptr ) {
        Log::warn( cc::warn( "Plugins scanner: can not open plugins directory " ) + cc::p( pluginsFolderPath ) );
        return result;
    }

    struct dirent* entry = nullptr;
    while( ( entry = ::readdir( dir ) ) != nullptr ) {
        if( entry->d_type != DT_DIR ) {
            if( entry->d_type == DT_LNK ) {
                char linkName[ 1024 ];
                std::string sourceName = pluginsFolderPath + std::string{ "/" } + entry->d_name;
                unsigned len = readlink( sourceName.c_str(), linkName, sizeof( linkName ) - 1 );
                if( len == -1 ) {
                    LOGGER_ERROR( "Symbolic link path too long" );
                    continue;
                }
                linkName[ len ] = '\0';

                struct stat statBuf = {};
                int statCode = stat( linkName, &statBuf );
                if( -1 == statCode || !S_ISDIR( statBuf.st_mode ) ) {
                    LOGGER_ERROR( cc::trace( "Symbolic link is not a directory: " ) + cc::note( entry->d_name ) );
                    continue;
                }

                LOGGER_TRACE_RAW( cc::trace( "Symbolic link is a directory: " ) + cc::note( entry->d_name ) );
            }
            else
                continue;
        }

        // NOTE: Ignore ".", ".." and hidden directories
        if( entry->d_name[ 0 ] == '.' )
            continue;

        result.push_back( entry->d_name );
    }
    ::closedir( dir );

    return result;
}

PluginUtils::StreamUPtr PluginUtils::getInternalPluginTemplate( const std::string& templateName ) const {
    const auto idSeparatorPos = templateName.find( ':' );
    if( idSeparatorPos == std::string::npos ) {
        Log::warn( cc::warn( "Cannot find separator ':' in '" ) + cc::note( templateName ) + cc::warn( "'" ) );
        return nullptr;
    }

    auto pluginId = templateName.substr( 0, idSeparatorPos );
    const auto filePathPos = idSeparatorPos + 1u + ( templateName[ idSeparatorPos + 1u ] == '/' );
    auto internalFilePath = templateName.substr( filePathPos );

    return getInternalPluginFile( pluginId, internalFilePath );
}

PluginUtils::StreamUPtr PluginUtils::getInternalPluginLuaScriptFile( const std::string& scriptId ) const {
    return getInternalPluginFile( getLuaScriptFilePath( scriptId ) );
}

PluginUtils::StreamUPtr PluginUtils::getInternalPluginFile( const std::string& pluginId,
                                                            const std::string& internalFilePath ) const {
    const auto fullPath = getPluginDirectory( pluginId ) + "/" + internalFilePath;
    return getInternalPluginFile( fullPath );
}

PluginUtils::StreamUPtr PluginUtils::getInternalPluginFileById( const std::string& fileId ) const {
    return getInternalPluginFile( getFilePathById( fileId ) );
}

std::string PluginUtils::extractPluginId( const std::string& scriptId ) const {
    std::string pluginId;

    std::tie( pluginId, std::ignore ) = splitScriptId( scriptId );

    return pluginId;
}

void PluginUtils::checkScriptFilePath( std::string& scriptName ) const {
    std::string searchStr = ".lua";

    auto luaPos = scriptName.find( searchStr );

    if( luaPos != std::string::npos && ( ( luaPos + searchStr.length() ) == scriptName.length() ) ) {
        scriptName.erase( luaPos );
    }

    if( scriptName.find( "HUB:" ) == std::string::npos ) {
        scriptName = "HUB:" + scriptName;
    }
}

std::string PluginUtils::getLuaScriptFilePath( const std::string& scriptId ) const {
    return getFilePathById( scriptId ) + ".lua";
}

std::string PluginUtils::getFilePathById( const std::string& fileId ) const {
    std::string pluginName;
    std::string internalScriptPath;
    std::tie( pluginName, internalScriptPath ) = splitScriptId( fileId );

    if( pluginName.empty() || internalScriptPath.empty() ) {
        Log::error( cc::error( "Plugin script path resolver: wrong plugin script path: " ) + cc::p( fileId ) );
        return {};
    }

    return getPluginDirectory( pluginName ) + "/" + internalScriptPath;
}

std::string PluginUtils::constructScriptName( const std::string& pluginId, const std::string& internalFileName ) const {
    const std::string prefix = m_pluginScriptsPrefix + pluginId;
    if( internalFileName.find( prefix ) == std::string::npos ) {
        return prefix + "/" + internalFileName;
    }

    return internalFileName;
}

PluginUtils::SplittedFileId PluginUtils::splitScriptId( const std::string& scriptId ) const {
    auto slashPos = scriptId.find( '/' );
    if( slashPos == std::string::npos )
        return {};

    auto prefixLength = strlen( m_pluginScriptsPrefix );
    std::string pluginId = scriptId.substr( prefixLength, slashPos - prefixLength );
    std::string internalScriptPath = scriptId.substr( slashPos + 1 );

    return std::make_pair( std::move( pluginId ), std::move( internalScriptPath ) );
}

PluginUtils::StreamUPtr PluginUtils::getInternalPluginFile( const std::string& internalFilePath ) const {
    auto stream = std::make_unique< std::ifstream >( internalFilePath );

    if( !stream->is_open() ) {
        Log::trace( cc::trace( "Plugin script reader: Could not find plugin file " ) + cc::p( internalFilePath ) +
                    cc::trace( " error: " ) + cc::error( strerror( errno ) ) );
        return nullptr;
    }

    return std::move( stream );
}

std::string PluginUtils::getPluginDirectory( const std::string& pluginId ) const {
    std::stringstream result;
    result << Environment::getPluginsFolder() << "/" << pluginId;
    struct stat buffer;
    if( (::stat( result.str().c_str(), &buffer ) != 0 ) || ( S_ISDIR( buffer.st_mode ) == 0 ) ) {
        result.str( {} );
        result << Environment::getCustomPluginsFolder() << "/" << pluginId;
    }
    return result.str();
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
