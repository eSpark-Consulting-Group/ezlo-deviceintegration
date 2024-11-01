#pragma once

#include "extensions_manager/extensions/plugins/base_config.h"

#include <memory>

namespace HubPlatform {
namespace ExtensionsManager {

class Environment;

class Plugin {
public:
    enum class Status { Stopped, Started };

public:
    Plugin( Environment& environment, std::unique_ptr< BaseConfig > baseConfig );
    virtual ~Plugin();

    const BaseConfig& getBaseConfig() const;

    void setStatus( Status status );
    Status getStatus() const;

    virtual void init();
    virtual void startup();

    virtual bool migrate();

    virtual bool teardown( const Json& operationInfo );
    virtual bool uninstall();

    bool hasExtensionFor( const std::string& pluginId, const std::string& extensionName = "" ) const;

protected:
    const Environment& getEnvironment() const;
    Environment& getEnvironment();

private:
    std::string constructScriptName( const std::string& script );

private:
    Environment& m_environment;
    std::unique_ptr< BaseConfig > m_baseConfig;
    Status m_status;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
