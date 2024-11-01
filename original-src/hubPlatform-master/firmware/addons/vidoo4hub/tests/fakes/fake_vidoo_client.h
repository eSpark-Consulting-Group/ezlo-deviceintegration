#pragma once

#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"
#include <vector>

namespace HubPlatform {
namespace UT {

class FakeVidooClient : public HubPlatform::Vidoo4Hub::IVidooClient {
public:
    void setUp() override{};
    void tearDown() override{};
    void sendMessage( const JsonObject& msg ) override {
        m_messages.emplace_back( msg );
    }

public:
    std::vector< JsonObject > m_messages;
};

}  // namespace UT
}  // namespace HubPlatform
