#include "uci_utils/uci_settings.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sstream>

using namespace ::testing;

namespace HubPlatform {
namespace UciUtils {
namespace UT {

class UciSettingsTestSuite : public Test {};

TEST_F( UciSettingsTestSuite, writeSectionToStream ) {
    const UciSection section{ "section",
                              "section_name",
                              { { "opt1", "1" }, { "opt2", "1 1" }, { "opt3", "1'1\"1\\" }, { "opt4", "" } },
                              { { "opt", "a" }, { "opt", "a a" }, { "opt", "" } } };

    std::ostringstream stream;
    stream << section;

    constexpr const char expected[] = "config section section_name\n"
                                      "\toption opt1 1\n"
                                      "\toption opt2 1\\ 1\n"
                                      "\toption opt3 1\\'1\\\"1\\\\\n"
                                      "\toption opt4 ''\n"
                                      "\tlist opt a\n"
                                      "\tlist opt a\\ a\n"
                                      "\tlist opt ''\n"
                                      "\n";

    EXPECT_EQ( stream.str(), expected );
}

TEST_F( UciSettingsTestSuite, writeSectionWithSoneInvalidFieldsToStream ) {
    const UciSection section{
        "section",
        "section name",
        { { "opt 1", "1" }, { "o'pt2", "1" }, { "op\"t3", "1" }, { "op\nt5", "1" }, { "", "1" }, { "opt7", "1" } },
        { { "", "" }, { "opt ", "a" } }
    };

    std::ostringstream stream;
    stream << section;

    constexpr const char expected[] = "config section\n"
                                      "\toption opt7 1\n"
                                      "\n";

    EXPECT_EQ( stream.str(), expected );
}

TEST_F( UciSettingsTestSuite, readFromEmptyStream ) {
    std::istringstream stream;
    UciSettings settings;
    stream >> settings;

    EXPECT_TRUE( settings.empty() );
    EXPECT_TRUE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readOneSection ) {
    constexpr const char sectionString[] = "config section '_section-name_'\n"
                                           "option opt1 1\n"
                                           "\toption 'opt2' '\" 2'\t\n"
                                           "   option \t\"opt3\"\t \"' 3\" \n"
                                           " \toption  opt4 \\ \\'\\\"\\\\ \n"
                                           "option opt5 ''\n"
                                           "option opt6 \"\"\n"
                                           "list 'opt' ' ' \n"
                                           "list opt \\ \n"
                                           "list opt ''";

    std::istringstream stream( sectionString );
    UciSettings settings;
    stream >> settings;

    const UciSection expectedSection{ "section",
                                      "_section-name_",
                                      { { "opt1", "1" },
                                        { "opt2", "\" 2" },
                                        { "opt3", "' 3" },
                                        { "opt4", " '\"\\" },
                                        { "opt5", "" },
                                        { "opt6", "" } },
                                      { { "opt", " " }, { "opt", " " }, { "opt", "" } } };

    EXPECT_EQ( settings, UciSettings( { expectedSection } ) );
    EXPECT_TRUE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readSeveralSections ) {
    constexpr const char settingsString[] = "\n"
                                            " package 'pkg'\n"
                                            "config sectionA \"a\"\n"
                                            "option opt1 aaa\n"
                                            "list 'opt' ' ' \n"
                                            "  \n"
                                            "  config sectionC \n"
                                            "option opt1 ccc\n"
                                            "  config sectionB B \n"
                                            "option opt1 bbb\n";

    std::istringstream stream( settingsString );
    UciSettings settings;
    stream >> settings;

    const UciSection sectionA{ "sectionA", "a", { { "opt1", "aaa" } }, { { "opt", " " } } };
    const UciSection sectionB{ "sectionB", "B", { { "opt1", "bbb" } }, {} };
    const UciSection sectionC{ "sectionC", "", { { "opt1", "ccc" } }, {} };

    EXPECT_EQ( settings, UciSettings( { sectionA, sectionC, sectionB } ) );
    EXPECT_TRUE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readSectionsWithComments ) {
    constexpr const char settingsString[] =
        "#header comment\n"
        " package 'pkg'  #package description\n"
        "config sectionA a'#'a #section 'a#a'\n"
        "option opt b\\#\"#\" \t # option with escape and quotes # \n #\\some description\\";

    std::istringstream stream( settingsString );
    UciSettings settings;
    stream >> settings;

    const UciSection sectionA{ "sectionA", "a#a", { { "opt", "b##" } }, {} };

    EXPECT_EQ( settings, UciSettings( { sectionA } ) );
    EXPECT_TRUE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readSettingsWithInvalidLinePrefix ) {
    constexpr const char settingsString[] = "config sectionA \"a\"\n"
                                            "option opt1 aaa\n"
                                            "bonfig sectionC 'c'\n"
                                            "option opt1 ccc\n";

    std::istringstream stream( settingsString );
    UciSettings settings;
    stream >> settings;

    EXPECT_EQ( settings, UciSettings() );
    EXPECT_FALSE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readSettingsWithInvalidLineStringsCount ) {
    constexpr const char settingsString[] = "config sectionA \"a\"\n"
                                            "option opt1 aaa\n"
                                            "config sectionC 'c c' c\n"
                                            "option opt1 ccc\n";

    std::istringstream stream( settingsString );
    UciSettings settings;
    stream >> settings;

    EXPECT_EQ( settings, UciSettings() );
    EXPECT_FALSE( stream.good() );
}

TEST_F( UciSettingsTestSuite, readSettingsWithInvalidPackageLinePosition ) {
    constexpr const char settingsString[] = "config sectionA \"a\"\n"
                                            "option opt1 aaa\n"
                                            "config sectionC 'c c' c\n"
                                            "package 'pkg'\n"
                                            "option opt1 ccc\n";

    std::istringstream stream( settingsString );
    UciSettings settings;
    stream >> settings;

    EXPECT_EQ( settings, UciSettings() );
    EXPECT_FALSE( stream.good() );
}

}  // namespace UT
}  // namespace UciUtils
}  // namespace HubPlatform
