#pragma once
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace CoreD {
namespace WalkTest {

class WalkTestInterface {
public:
    explicit WalkTestInterface( Services::Service& hostService );
    JsonObject protectSystemTestStart( const JsonObject& params );
    JsonObject protectSystemTestStop( const JsonObject& params );
    JsonObject protectSystemTestExtend( const JsonObject& params );

private:
    Services::Service& m_hostService;
};

}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform
