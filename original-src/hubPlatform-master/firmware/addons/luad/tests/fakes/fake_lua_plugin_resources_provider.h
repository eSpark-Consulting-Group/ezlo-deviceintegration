#pragma once

#include "luad/interpreter/module_loader/lua_module_loader.h"

#include "common_utils/cpp14.h"

#include <memory>
#include <string>
#include <sstream>

namespace HubPlatform {
namespace Luad {
namespace UT {

class FakeLuaPluginResourcesProvider final : public LuaPluginResourcesProvider {
public:
    bool isPluginScript( const std::string& scriptName ) const override {
        return scriptName.find( "HUB" ) == 0U;
    }

    bool isAnonymousPluginScript( const std::string& scriptName ) const override {
        return scriptName.find( "anonymous:" ) == 0U;
    }

    std::unique_ptr< std::istream > getScript( const std::string& scriptId ) override {
        if( scriptId == "HUB:ordinary_require_aware_script" ) {
            return std::make_unique< std::stringstream >( "local r = require \"HUB:2\"\n"
                                                          "if (r.id ~= 5) then error('Invalid r') end\n"
                                                          "if (r.number ~= 4) then error('Invalid r') end\n" );
        }
        else if( scriptId == "HUB:2" ) {
            return std::make_unique< std::stringstream >( "local r = require \"HUB:3\"\n"
                                                          "r.id = 5\n"
                                                          "return r\n" );
        }
        else if( scriptId == "HUB:3" ) {
            return std::make_unique< std::stringstream >( "local r = {}"
                                                          "r.id = 1\n"
                                                          "r.number = 4\n"
                                                          "return r\n" );
        }
        else if( scriptId == "HUB:try_to_make_failing_require" ) {
            return std::make_unique< std::stringstream >( R"( require "HUB:trash" )" );
        }
        else if( scriptId == "HUB:a" ) {
            return std::make_unique< std::stringstream >( R"( print( "hello cruel world" ) )" );
        }
        else if( scriptId == "HUB:b" ) {
            return std::make_unique< std::stringstream >( R"( require "anonymous:a" )" );
        }
        else if( scriptId == "anonymous:a" ) {
            return std::make_unique< std::stringstream >( R"( print( "hello cruel world" ) )" );
        }
        else if( scriptId == "HUB:dynamic_module_that_does_not_exist" ) {
            return std::make_unique< std::stringstream >(
                R"( local module = require( "dynamic_module_that_does_not_exist" )
                    return module )" );
        }

        return nullptr;
    }

    std::string getScriptFilePath( const std::string& /*scriptId*/ ) const override {
        return "";
    }

    std::string getDataFilePath( const std::string& /*scriptId*/ ) const override {
        return "";
    }

    std::string getPluginIdByScriptId( const std::string& /*scriptId*/ ) const override {
        return m_pluginId;
    }
    bool checkPluginPermission( const std::string& /*pluginId*/, const std::string& moduleToCheck ) const override {
        if( m_ignorePluginPermissionCheck ) {
            return true;
        }

        if( moduleToCheck == "dynamic_module_that_does_not_exist" )
            return true;

        if( moduleToCheck == "storage" )
            return true;
        return false;
    }

    void ignorePluginPermissionCheck( bool value ) {
        m_ignorePluginPermissionCheck = value;
    }

    void setupPluginId( const std::string& pluginId ) {
        m_pluginId = pluginId;
    }

private:
    std::string m_pluginId;
    bool m_ignorePluginPermissionCheck = false;
};

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
