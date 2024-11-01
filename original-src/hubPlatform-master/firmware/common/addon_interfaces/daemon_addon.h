#pragma once

#include "addon.h"

#include "daemon_module/daemon_module.h"

#include <utility>

namespace HubPlatform {
namespace Addons {

template < typename AddonType >
class DaemonAddon final : public DaemonModule {
    static_assert( std::is_base_of< Addon, AddonType >::value, "AddonType should be inherited from Addon interface" );

public:
    template < typename... Args >
    DaemonAddon( Args&&... args )
    : m_addon( std::forward< Args >( args )... ) {}

private:
    void runImpl() override {
        m_addon.run();
    }

    void setBinaryName( const std::string& name ) override {
        m_addon.setBinaryName( name );
    }

    int restoreBackup( const std::string& tempBackupFolder ) override {
        return m_addon.restoreBackup( tempBackupFolder );
    }

    void onTerminate() override {
        m_addon.stop( Addon::StoppingMode::OnTerminationSignalReceived );
    }

    void defineDaemonParameters( tiny::command_line::parser& parser ) override {
        m_addon.defineDaemonParameters( parser );
    }

private:
    AddonType m_addon;
};
}  // namespace Addons
}  // namespace HubPlatform
