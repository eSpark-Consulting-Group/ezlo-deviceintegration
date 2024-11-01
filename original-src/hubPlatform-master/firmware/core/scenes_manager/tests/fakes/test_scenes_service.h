#pragma once

#include "scenes_manager/scenes_service.h"

namespace HubPlatform {
namespace ScenesManager {

class TestService : public ScenesManager::Service {
public:
    TestService();
    TestService( const std::string& name,
                 std::unique_ptr< Services::BusWrapper > busWrapper,
                 std::unique_ptr< IDbClient > dbClient );

    void setUp() override;
    void tearDown() override;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
