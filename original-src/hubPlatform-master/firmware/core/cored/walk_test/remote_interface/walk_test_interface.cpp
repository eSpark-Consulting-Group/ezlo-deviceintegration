#include "walk_test_interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {
namespace WalkTest {

WalkTestInterface::WalkTestInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject WalkTestInterface::protectSystemTestStart( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODES_PROTECT_SYSTEM_TEST_START );
}

JsonObject WalkTestInterface::protectSystemTestStop( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODES_PROTECT_SYSTEM_TEST_STOP );
}

JsonObject WalkTestInterface::protectSystemTestExtend( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODES_PROTECT_SYSTEM_TEST_EXTEND );
}

}  // namespace WalkTest
}  // namespace CoreD
}  // namespace HubPlatform
