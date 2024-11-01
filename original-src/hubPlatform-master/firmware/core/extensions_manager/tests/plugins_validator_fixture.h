#include "common_utils/cpp14.h"
#include "bus/fake_bus.h"
#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "database_client/fake_db/fake_db_client.h"
#include "extensions_manager/plugin_model/plugins_metedata_validator.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

#include <gmock/gmock.h>

#define SHOULD_THROW( expr, ExceptionType, message, file, line ) \
    try {                                                        \
        expr                                                     \
    }                                                            \
    catch( const ExceptionType& ex ) {                           \
        SUCCEED() << ex.what();                                  \
        return;                                                  \
    }                                                            \
    FAIL() << message << ", at: " << file << ":" << line;

#define SHOULD_NOT_THROW( expr, ExceptionType, message, file, line ) \
    try {                                                            \
        expr SUCCEED();                                              \
    }                                                                \
    catch( const ExceptionType& ex ) {                               \
        FAIL() << message << ", at: " << file << ":" << line;        \
        return;                                                      \
    }

namespace ExtensionsManager {
namespace UT {

using ::testing::AtLeast;
using ::testing::Return;
using namespace HubPlatform::Tests;
using namespace HubPlatform::ExtensionsManager;

namespace Resources {
extern const std::string pluginDataJsonConfig;
extern const std::string pluginDataJsonInterfaces;

extern const char* const PluginPrefix;
extern const char* const PluginPrefixedName;
}  // namespace Resources

class FakeService final : public HubPlatform::Services::Service {
public:
    FakeService()
    : HubPlatform::Services::Service(
          "TestService",
          std::make_unique< HubPlatform::Services::BusWrapper >(
              HubPlatform::Services::BusWrapper::BusTypeTag< HubPlatform::Services::UT::FakeResponseBus >{} ),
          std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    void setUp() override {
        HubPlatform::Services::Service::setUp();
    }

    void tearDown() override {
        HubPlatform::Services::Service::tearDown();
    }

    BaseBus* getBus() {
        return HubPlatform::Services::Service::getBus();
    }
};

class PluginsValidatorFixture : public testing::Test {
public:
    void SetUp() override {
        m_service.setUp();
        m_service.getDBClient().clean();

        m_pluginsMetadataValidator = std::make_unique< PluginsMetadataValidator >();
        m_remoteInterface = std::make_unique< RemoteInterface >( m_service );
    }

    void TearDown() override {}

    PluginsMetadataValidator& getPluginsMetadataValidator() {
        return *m_pluginsMetadataValidator;
    }

    RemoteInterface& getRemoteInterface() {
        return *m_remoteInterface;
    }

    std::string createConfigMeta( std::string meta,
                                  std::string name,
                                  std::string desc,
                                  std::string auth,
                                  std::string pref,
                                  std::string size,
                                  std::string type,
                                  std::string lang,
                                  std::string vers,
                                  std::string plac,
                                  std::string id );

    std::string createInterfaces( std::string config,
                                  std::string script,
                                  std::string field,
                                  std::string format,
                                  std::string field_type,
                                  std::string name,
                                  std::string desc,
                                  std::string type,
                                  std::string req );

    bool checkDbItem( const JsonObject& json,
                      const std::string& author = "Ezlo Innovation",
                      const char* file = "",
                      int line = -1 );

private:
    std::unique_ptr< PluginsMetadataValidator > m_pluginsMetadataValidator;
    std::unique_ptr< RemoteInterface > m_remoteInterface;

protected:
    FakeService m_service;
};

}  // namespace UT
}  // namespace ExtensionsManager
