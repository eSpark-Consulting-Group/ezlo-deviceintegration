#pragma once

#include "service.h"

#include "daemon_module/daemon_module.h"

#include <utility>

namespace HubPlatform {
namespace Services {

template < typename ServiceType >
class DaemonService final : public DaemonModule {
    static_assert( std::is_base_of< Service, ServiceType >::value,
                   "ServiceType should be inherited from Service interface" );

public:
    template < typename... Args >
    DaemonService( Args&&... args )
    : m_service( std::forward< Args >( args )... ) {}

private:
    void runImpl() override {
        m_service.run();
    }

    void setBinaryName( const std::string& name ) override {
        m_service.setBinaryName( name );
    }

    int restoreBackup( const std::string& tempBackupFolder ) override {
        return m_service.restoreBackup( tempBackupFolder );
    }

    void onTerminate() override {
        m_service.stop( Service::StoppingMode::OnTerminationSignalReceived );
    }

    void defineDaemonParameters( tiny::command_line::parser& parser ) override {
        m_service.defineDaemonParameters( parser );
    }

private:
    ServiceType m_service;
};
}  // namespace Services
}  // namespace HubPlatform
