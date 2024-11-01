#include "extensions_manager/plugin_model/plugin_package_validator.h"
#include "extensions_manager/plugin_model/plugins_metedata_validator.h"
#include "extensions_manager/plugin_model/plugins_gateway_validator.h"
#include "extensions_manager/plugin_model/model_resources.h"

#include "plugin_utils/plugin_utils.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"

#include <fstream>

namespace HubPlatform {
namespace ExtensionsManager {

using namespace ::ExtensionsManager;

PluginPackageValidatorError::PluginPackageValidatorError( std::string message )
: runtime_error( std::move( message ) ) {}

PluginPackageValidator::PluginPackageValidator( const std::string& packagePath, const std::string& pluginName )
: m_packagePath( packagePath )
, m_pluginName( pluginName )
, m_packageDearchivePrefix( Environment::getTempDataFolder() + "/plugin_package_validator/" ) {}

void PluginPackageValidator::validate() {
    std::string packageDearchivePath;

    try {
        packageDearchivePath = filesystem::createRandomFolder( m_packageDearchivePrefix );
        System::untarArchive( m_packagePath, packageDearchivePath );

        std::string configFilePath = packageDearchivePath + "/" + Resources::PluginFiles::ConfigFileName;
        std::string interfaceFilePath = packageDearchivePath + "/" + Resources::PluginFiles::InterfaceFileName;
        std::function< bool( std::string ) > isFileExist = [packageDearchivePath]( const std::string& fileId ) {
            std::string componentName;
            std::string relativeFilePath;
            PluginUtils pluginUtils;

            std::tie( componentName, relativeFilePath ) = pluginUtils.splitScriptId( fileId );
            if( componentName.empty() || relativeFilePath.empty() ) {
                return false;
            }

            return filesystem::isFileExist( packageDearchivePath + "/" + relativeFilePath );
        };

        // TODO: check other aspects of a plugin

        PluginsMetadataValidator metadataValidator;
        std::ifstream configFile( configFilePath );
        std::ifstream interfaceFile( interfaceFilePath );
        metadataValidator.parse( configFile, interfaceFile, isFileExist );

        auto config = metadataValidator.getConfig();

        JsonPath path;
        const auto id = json_get_str( config, "id" );
        if( !m_pluginName.empty() && m_pluginName != id ) {
            throw PluginsMetadataValidatorException( "Plugin config 'id' doesn't match plugin name", path );
        }

        PluginsGatewayValidator gatewayValidator;
        gatewayValidator.process( config, isFileExist );

        filesystem::deleteFolder( packageDearchivePath );
    }
    catch( const PluginsMetadataValidatorException& ex ) {
        filesystem::deleteFolder( packageDearchivePath );
        throw;
    }
    catch( const PluginsGatewayValidatorException& ex ) {
        filesystem::deleteFolder( packageDearchivePath );
        throw;
    }
    catch( const std::exception& ex ) {
        filesystem::deleteFolder( packageDearchivePath );
        throw PluginPackageValidatorError( ex.what() );
    }
    catch( ... ) {
        filesystem::deleteFolder( packageDearchivePath );
        throw PluginPackageValidatorError( "" );
    }
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
