#pragma once

#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class VidooBusClient : public IVidooClient {
public:
    VidooBusClient( Services::Service& service, MessageCallback callback );

    void setUp() override;
    void tearDown() override;
    void sendMessage( const JsonObject& msg ) override;

private:
    Services::Service& m_service;
    MessageCallback m_callback;  // TODO check if callback is needed
    // TODO ?? add process method
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
