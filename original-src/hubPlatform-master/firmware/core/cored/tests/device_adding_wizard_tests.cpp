
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "cored/tests/service/test_core_service.h"
#include "cored/tests/tests_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "common_utils/cpp14.h"
#include "base_utils/utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Model::Resources::DBNames;

class DeviceAddingWizardTestSuite : public ::testing::Test {};

TEST_F( DeviceAddingWizardTestSuite, getCategoriesEmptyFile ) {
    DeviceAddingWizard deviceAddingWizard( resources::FileDeviceAddingWizard );
    DeviceAddingWizard::Categories categories = deviceAddingWizard.getCategories();
    ASSERT_FALSE( categories.second );
    JsonArray shouldBe = JSON( "error" << JSON( "message"
                                                << "source json file isn't opened" ) );
    EXPECT_EQ( categories.first, shouldBe );
}

TEST_F( DeviceAddingWizardTestSuite, getCategoriesInvalidJson ) {
    utils::writeToFile( resources::FileDeviceAddingWizard, "{" );
    DeviceAddingWizard deviceAddingWizard( resources::FileDeviceAddingWizard );
    DeviceAddingWizard::Categories categories = deviceAddingWizard.getCategories();
    ASSERT_FALSE( categories.second );
    JsonArray shouldBe = JSON( "error" << JSON( "message"
                                                << "invalid json" ) );
    EXPECT_EQ( categories.first, shouldBe );
    remove( resources::FileDeviceAddingWizard );
}

TEST_F( DeviceAddingWizardTestSuite, getCategoriesTestReturnedCategories ) {
    utils::writeToFile( resources::FileDeviceAddingWizard, resources::daw::SourceJson.dump() );
    DeviceAddingWizard deviceAddingWizard( resources::FileDeviceAddingWizard );
    DeviceAddingWizard::Categories categories = deviceAddingWizard.getCategories();
    remove( resources::FileDeviceAddingWizard );
    ASSERT_TRUE( categories.second );
    EXPECT_EQ( categories.first, resources::daw::CategoriesList );
}

TEST_F( DeviceAddingWizardTestSuite, getDevices ) {
    utils::writeToFile( resources::FileDeviceAddingWizard, resources::daw::SourceJson.dump() );
    DeviceAddingWizard deviceAddingWizard( resources::FileDeviceAddingWizard );
    DeviceAddingWizard::Categories categories = deviceAddingWizard.getDevices();
    ASSERT_TRUE( categories.second );
    EXPECT_EQ( categories.first, resources::daw::DeviceList );

    categories = deviceAddingWizard.getDevices( "ALARMS" );
    ASSERT_TRUE( categories.second );
    EXPECT_EQ( categories.first, resources::daw::AlarmDeviceList );
    remove( resources::FileDeviceAddingWizard );
}

}  // namespace UT
}  // namespace HubPlatform
