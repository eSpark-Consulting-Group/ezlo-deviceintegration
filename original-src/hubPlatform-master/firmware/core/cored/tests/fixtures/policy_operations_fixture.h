#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace CoreD {
class PolicyOperationsHandler;
}

namespace UT {

class PolicyOperationsFixture : public Tests::CommonNmaTests {
public:
    using Service = HubPlatform::CoreD::CoreService;
    using RemoteInterface = HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface;
    // using KeyStringList = std::vector< std::pair< std::string, std::string > >;

    void SetUp() override;
    void TearDown() override;

    void addPolicy( const std::string& policyId, const std::string& name );
    void addPolicyJSON( const Json& obj );
    void deletePolicy( const std::string& policyId );
    JsonObject validatePolicyCreate( JsonObject& message );
    JsonObject getPolicyUpdateBroadcastData( const JsonObject& updated, const JsonObject& saved );
    JsonObject getPolicyList( const JsonObject& params );
    JsonObject getPolicyGet( const JsonObject& params );

protected:
    JsonObject replaceTokens( const JsonObject& json, const KeyStringList& replaceList ) {
        return replaceTokens( json.dump(), replaceList );
    }

    Services::Service& getService();

protected:
    std::unique_ptr< HubPlatform::CoreD::PolicyOperationsHandler > m_policyOperationsHandler;
    std::unique_ptr< CoreD::Tests::TestCoreService > m_service;
    std::unique_ptr< RemoteInterface > m_remoteInterface;
    JsonObject m_result;
};
}  // namespace UT
}  // namespace HubPlatform
