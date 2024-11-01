#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/house_modes/house_mode_utils.h"

#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/house_modes/house_mode_utils.h"

#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace CoreD {
class GroupOperationsHandler;
}

namespace UT {

class GroupOperationsFixture : public Tests::CommonNmaTests {
public:
    using Service = HubPlatform::CoreD::CoreService;
    using RemoteInterface = HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface;
    // using KeyStringList = std::vector< std::pair< std::string, std::string > >;

    void SetUp() override;
    void TearDown() override;

    void addDevice( const std::string& deviceId );
    void addItemGroup( const std::string& groupId, const std::string& name );
    void addDeviceGroup( const std::string& groupId, const std::string& name );
    JsonObject getDeviceGroupsList( const JsonObject& params );
    JsonObject getDeviceGroupGet( const JsonObject& params );
    JsonObject validateDeviceGroupCreate( JsonObject& message );
    JsonObject validateItemGroupCreate( JsonObject& message );
    JsonObject getDeviceGroupUpdateBroadcastData( const JsonObject& updated, const JsonObject& saved );
    JsonObject getItemGroupUpdateBroadcastData( const JsonObject& updated, const JsonObject& saved );
    JsonObject getItemGroupsList( const JsonObject& params );
    JsonObject getItemGroupGet( const JsonObject& params );
    JsonObject getDeviceGroupFind( const JsonObject& params );
    JsonObject deviceItemGroupExpand( const JsonObject& params );
    JsonObject createDeviceGroupRequest( const std::string& name,
                                         const std::string& category,
                                         const std::string& devices,
                                         const std::string& exceptions );
    void addDeviceGroupWithCategoriesAndDevices( const std::string& groupId,
                                                 const std::string& name,
                                                 const JsonArray& categories,
                                                 const JsonArray& devices,
                                                 const JsonArray& exceptions );

protected:
    JsonObject replaceTokens( const JsonObject& json, const KeyStringList& replaceList ) {
        return replaceTokens( json.dump(), replaceList );
    }

    Services::Service& getService();

protected:
    std::unique_ptr< HubPlatform::CoreD::GroupOperationsHandler > m_groupOperationsHandler;
    std::unique_ptr< CoreD::Tests::TestCoreService > m_service;
    std::unique_ptr< RemoteInterface > m_remoteInterface;
    JsonObject m_result;
};
}  // namespace UT
}  // namespace HubPlatform
