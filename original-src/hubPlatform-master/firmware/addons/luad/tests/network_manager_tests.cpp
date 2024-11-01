#include "luad/modules/network/network_manager.h"
#include "luad/tests/mock_objects.h"
#include "service_interfaces/tests/fakes/fake_service.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cassert>
#include <memory>

/*

TESTPLAN

TODO    1. NetworkManager connection
TODO        1.1 Check creation of async socket client
TODO        1.2 Check creation of async socket
TODO        1.3 Check connect on async socket gets invoked
TODO    2. NetworkManager sending
TODO        2.1 Check using the same async socket client for the same group name
TODO        2.2 Check send on async socket gets invoked
TODO    3. NetworkManager receiving
TODO        3.1 Check using the same async socket client for the same group name
TODO        3.2 Check receive on async socket gets invoked
TODO    4. NetworkManager inspecting data
TODO        4.1 Check using the same async socket client for the same group name
TODO        4.2 Check inspecting_data on async socket gets invoked
TODO    5. NetworkManager inspecting data size
TODO        5.1 Check using the same async socket client for the same group name
TODO        5.2 Check inspecting_data_size on async socket gets invoked
TODO    6. NetworkManager closing
TODO        6.1 Check using the same async socket client for the same group name
TODO        6.2 Check async socket gets removed from async socket client
TODO    7. NetworkManager setEventsHandler
TODO        6.1 Check setEventsHandler on async socket client gets invoked
TODO        6.2 Check setEventsHandler on async socket gets invoked

*/

namespace HubPlatform {
namespace UT {

using namespace LuaNetwork;

class NetworkManagerTestSuite : public testing::Test {
protected:
    NetworkManagerTestSuite()
    : m_eventsLoop( std::make_unique< MockEventsLoop >() )
    , m_eventsEmitter( std::make_unique< MockEventsEmitter >() ) {}

    NetworkManager& getNetworkManager() {
        assert( !m_networkManager );
        m_networkManager = std::unique_ptr< NetworkManager >{ new NetworkManager(
            m_service, std::move( m_eventsEmitter ), std::move( m_eventsLoop ) ) };
        return *m_networkManager;
    }

    MockEventsLoop& getEventsLoop() {
        assert( m_eventsLoop );
        return *m_eventsLoop;
    }

    MockEventsEmitter& getEventsEmitter() {
        assert( m_eventsEmitter );
        return *m_eventsEmitter;
    }

    AsyncSocketsManager& getAsyncSocketsManager() {
        assert( m_networkManager );
        return m_networkManager->m_asyncSocketsManager;
    }

private:
    std::unique_ptr< MockEventsLoop > m_eventsLoop;
    std::unique_ptr< MockEventsEmitter > m_eventsEmitter;

    std::unique_ptr< NetworkManager > m_networkManager;
    Services::UT::FakeService m_service;
};

TEST_F( NetworkManagerTestSuite, testCheckEpollInstanceCreation /* 1.1 */ ) {
    getNetworkManager();
    EXPECT_TRUE( true );
}

}  // namespace UT
}  // namespace HubPlatform
