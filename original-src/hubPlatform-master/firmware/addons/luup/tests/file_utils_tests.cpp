
#include "luup/vera_plugins/file_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

class FileUtilsTestSuite : public Test {};

TEST_F( FileUtilsTestSuite, testGetDir ) {
    EXPECT_EQ( "/test/vera/", FileUtils::getDir( "/test/vera/file1.txt" ) );
    EXPECT_EQ( "test/vera/", FileUtils::getDir( "test/vera/file2.txt" ) );
    EXPECT_EQ( "", FileUtils::getDir( "file3.txt" ) );
    EXPECT_EQ( "", FileUtils::getDir( "/file4.txt/" ) );
    EXPECT_EQ( "/", FileUtils::getDir( "/file5.txt" ) );
    EXPECT_EQ( "", FileUtils::getDir( "file6.txt/" ) );
    EXPECT_EQ( "", FileUtils::getDir( "/" ) );
}
