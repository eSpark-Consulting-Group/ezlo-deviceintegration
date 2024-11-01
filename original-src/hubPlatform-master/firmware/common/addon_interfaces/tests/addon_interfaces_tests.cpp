#include "addon_interfaces/addon.h"
#include "service_interfaces/bus_wrapper.h"

#include "bus/base_bus.h"
#include "common_utils/cpp14.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include "test_utils/test_utils.h"
#include "system_utils/environment.h"
#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

using namespace ::testing;

class TestBus final : public BaseBus {
public:
    TestBus( const std::string& privateName, const RxCallBack& rxCallBack, bool silent, int connectionAttempts )
    : BaseBus( privateName, rxCallBack ) {}

    void start() override {}

    void stop() override {}

    void join( const char* /*group*/ ) override {}

    bool multicast( const char* /*group*/, int /*msgClass*/, const std::string& /*msg*/ ) override {
        return true;
    }

    bool isRegularMessage( int msgType ) const override {
        return true;
    }
};

class TestAddon final : public Addons::Addon {
public:
    TestAddon()
    : Addons::Addon( "test_addon",
                     std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< TestBus >{} ),
                     std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

protected:
    CommonUtils::AddonVersion getVersion() const override {
        return CommonUtils::AddonVersion{ 1, 0, "42" };
    }
};

class AddonInterfacesTestSuite : public Test {
public:
    void SetUp() override {
        m_addon = std::make_unique< TestAddon >();
    }

    TestAddon& getAddon() {
        return *m_addon;
    }

protected:
    Tests::TemporaryRootFolderWrapper m_folderWrapper;
    std::unique_ptr< TestAddon > m_addon;
};

}  // namespace UT
}  // namespace HubPlatform
