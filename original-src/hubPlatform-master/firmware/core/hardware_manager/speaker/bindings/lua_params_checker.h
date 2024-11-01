#pragma once

#include "service_interfaces/service.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

#include "json_utils/json_utils.h"

#include <string>
#include <utility>
#include <sys/stat.h>

namespace HubPlatform {

/** Example mixin. Specify own classes to throw. */
/*
struct Mixin {
    using RequiredParameter = std::runtime_error;
    using NoFile = std::runtime_error;
    using FilePermissions = std::runtime_error;
    using RequestPermissions = std::runtime_error;
    using WrongParameters = std::runtime_error;
    using ParameterLimit = std::runtime_error;
    using TotalLimit = std::runtime_error;
    using NoObject = std::runtime_error;
};
*/

/** Code common to many lua modules: check plugin permissions on files, objects, wrong parameters, limits, etc */
template < class TErrors >
class LuaParamsChecker {
public:
    using Errors = TErrors;

public:
    LuaParamsChecker( Services::Service& service, ExtensionsManager::RemoteInterface& remote, std::string pluginId );
    LuaParamsChecker( const LuaParamsChecker& ) = delete;
    LuaParamsChecker& operator=( const LuaParamsChecker& ) = delete;
    LuaParamsChecker( LuaParamsChecker&& ) = delete;
    LuaParamsChecker& operator=( LuaParamsChecker&& ) = delete;

public:
    void checkPluginPermissions( const std::string& pluginId, const std::string& script );
    void checkFullPathExists( const std::string& script, const std::string& nameForException );
    std::string makeFullPath( const std::string& script, bool isLuaScript = true );
    std::string checkFileAndMakeFullPath( const std::string& pluginId, const std::string& script );

    std::string checkRequiredString( const Json& params, const std::string& key );

protected:
    Services::Service& m_hostService;
    ExtensionsManager::RemoteInterface& m_remote;
    std::string m_pluginId;
};

template < class TErrors >
inline LuaParamsChecker< TErrors >::LuaParamsChecker( Services::Service& service,
                                                      ExtensionsManager::RemoteInterface& remote,
                                                      std::string pluginId )
: m_hostService( service )
, m_remote( remote )
, m_pluginId( std::move( pluginId ) ) {}

template < class TErrors >
inline void LuaParamsChecker< TErrors >::checkPluginPermissions( const std::string& pluginId,
                                                                 const std::string& script ) {
    if( script.empty() )
        return;

    auto foundPluginId = m_remote.getPluginIdByScriptId( script );

    if( foundPluginId != pluginId ) {
        throw typename Errors::FilePermissions( script );
    }
}

template < class TErrors >
inline std::string LuaParamsChecker< TErrors >::checkFileAndMakeFullPath( const std::string& pluginId,
                                                                          const std::string& script ) {
    if( !script.empty() ) {
        checkPluginPermissions( pluginId, script );

        auto fullPath = makeFullPath( script );
        checkFullPathExists( fullPath, script );
        return fullPath;
    }
    return script;
}

template < class TErrors >
std::string LuaParamsChecker< TErrors >::makeFullPath( const std::string& script, bool isLuaScript ) {
    auto fullScriptPath = m_remote.getLuaScriptFilePath( script );
    if( !isLuaScript ) {
        const char ext[] = ".lua";
        const size_t extLen = sizeof( ext ) - 1;
        if( !fullScriptPath.empty() && fullScriptPath.rfind( ext ) == fullScriptPath.size() - extLen )
            return fullScriptPath.substr( 0, fullScriptPath.size() - extLen );
    }
    return fullScriptPath;
}

template < class TErrors >
void LuaParamsChecker< TErrors >::checkFullPathExists( const std::string& script,
                                                       const std::string& nameForException ) {
    if( script.empty() )
        return;

    struct stat fileStat = { 0 };
    if( -1 == stat( script.c_str(), &fileStat ) ) {
        const auto& toReport = nameForException.empty() ? script : nameForException;
        throw typename Errors::NoFile( toReport );
    }
}

template < class TErrors >
std::string LuaParamsChecker< TErrors >::checkRequiredString( const Json& params, const std::string& key ) {
    Json value;
    if( !json_get( params, key, value ) )
        throw typename Errors::RequiredParameter( key );
    if( value.empty() || !value.is_string() )
        throw typename Errors::WrongParameterFormat( key );
    return value.get< std::string >();
}

}  // namespace HubPlatform
