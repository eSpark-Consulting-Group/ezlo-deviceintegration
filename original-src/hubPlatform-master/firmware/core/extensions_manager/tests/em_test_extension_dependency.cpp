#include "extensions_manager/extensions/extension_dependency.h"

#include "common_utils/addons/addon_summary.h"
#include "plugin_utils/plugin_summary.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Addon dependencies
DONE        1.1 Same major, same minor, any build
DONE        1.2 Same major, lower minor
DONE        1.3 Same major, higher minor
DONE        1.4 Lower major, any minor
DONE        1.5 Higher major, any minor
DONE        1.6 Other addon id
DONE    2. Plugin dependencies
DONE        2.1 Same major, same minor
DONE        2.2 Same major, lower minor
DONE        2.3 Same major, higher minor
DONE        2.4 Lower major, any minor
DONE        2.5 Higher major, any minor
DONE        2.6 Other addon id

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::ExtensionsManager;

class AddonDependencyTestSuite : public Test {
public:
    AddonDependency dependency( const JsonObject& json ) {
        return AddonDependency::fromJson( json );
    }

    CommonUtils::AddonSummary addon( const JsonObject& json ) {
        return CommonUtils::AddonSummary::fromJson( json );
    }
};

TEST_F( AddonDependencyTestSuite, testSameMajorSameMinorAnyBuild /*1.1*/ ) {
    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.2" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2.0" ) ) ) );

    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.2" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2.386" ) ) ) );
}

TEST_F( AddonDependencyTestSuite, testSameMajorLowerMinor /*1.2*/ ) {
    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.1" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2" ) ) ) );
}

TEST_F( AddonDependencyTestSuite, testSameMajorHigherMinor /*1.3*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "4.3" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );
}

TEST_F( AddonDependencyTestSuite, testLowerMajorAnyMinor /*1.4*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.1" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.3" ) ) ) );
}

TEST_F( AddonDependencyTestSuite, testHigherMajorAnyMinor /*1.5*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.1" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.3" ) ) ) );
}

TEST_F( AddonDependencyTestSuite, testOtherAddonId /*1.6*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "4.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "lua"
                                                   << "version"
                                                   << "4.2" ) ) ) );
}

class PluginDependencyTestSuite : public Test {
public:
    PluginDependency dependency( const JsonObject& json ) {
        return PluginDependency::fromJson( json );
    }

    CommonUtils::PluginSummary addon( const JsonObject& json ) {
        return CommonUtils::PluginSummary::fromJson( json );
    }
};

TEST_F( PluginDependencyTestSuite, testSameMajorSameMinor /*2.1*/ ) {
    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.2" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2.0" ) ) ) );

    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.2" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2.386" ) ) ) );
}

TEST_F( PluginDependencyTestSuite, testSameMajorLowerMinor /*2.2*/ ) {
    EXPECT_TRUE( dependency( JSON( "id"
                                   << "zwave"
                                   << "version"
                                   << "4.1" ) )
                     .isSatisfiedBy( addon( JSON( "id"
                                                  << "zwave"
                                                  << "version"
                                                  << "4.2" ) ) ) );
}

TEST_F( PluginDependencyTestSuite, testSameMajorHigherMinor /*2.3*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "4.3" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );
}

TEST_F( PluginDependencyTestSuite, testLowerMajorAnyMinor /*2.4*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.1" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "3.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.3" ) ) ) );
}

TEST_F( PluginDependencyTestSuite, testHigherMajorAnyMinor /*2.5*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.1" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.2" ) ) ) );

    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "5.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "zwave"
                                                   << "version"
                                                   << "4.3" ) ) ) );
}

TEST_F( PluginDependencyTestSuite, testOtherAddonId /*2.6*/ ) {
    EXPECT_FALSE( dependency( JSON( "id"
                                    << "zwave"
                                    << "version"
                                    << "4.2" ) )
                      .isSatisfiedBy( addon( JSON( "id"
                                                   << "bluetooth"
                                                   << "version"
                                                   << "4.2" ) ) ) );
}
