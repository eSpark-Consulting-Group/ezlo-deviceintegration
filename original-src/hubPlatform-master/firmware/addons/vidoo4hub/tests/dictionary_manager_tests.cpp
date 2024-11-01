#include <gtest/gtest.h>

#include "vidoo4hub/dictionary_manager/dictionary_manager.h"
#include "vidoo4hub/tests/fakes/fake_dictionary_manager.h"

/*

TESTPLAN

DONE    1. Fake dictionary manager tests
DONE        1.1 Write dictionary
DONE        1.2 Write empty dictionary
DONE        1.3 Read dictionary
DONE        1.4 Clear dictionary

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class FakeDictionaryManagerTestSuite : public ::testing::Test {};

TEST_F( FakeDictionaryManagerTestSuite, writeDictionary ) /* 1.1 */ {
    FakeDictionaryManager dictionaryManager;

    const std::string dictionaryId1 = "dictionary_id_1";
    const std::string dictionaryId2 = "dictionary_id_2";
    auto testJson1 = Json( "Test Json 1" );
    auto testJson2 = Json( "Test Json 2" );

    dictionaryManager.updateDictionary( dictionaryId1, testJson1 );
    dictionaryManager.updateDictionary( dictionaryId2, testJson2 );

    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 2 );

    auto it1 = dictionaryManager.m_dictionaryMap.find( dictionaryId1 );
    ASSERT_TRUE( it1 != dictionaryManager.m_dictionaryMap.end() );
    EXPECT_EQ( it1->second.dump(), testJson1.dump() );

    auto it2 = dictionaryManager.m_dictionaryMap.find( dictionaryId2 );
    ASSERT_TRUE( it2 != dictionaryManager.m_dictionaryMap.end() );
    EXPECT_EQ( it2->second.dump(), testJson2.dump() );
};

TEST_F( FakeDictionaryManagerTestSuite, writeEmptyDictionary ) /* 1.2 */ {
    FakeDictionaryManager dictionaryManager;

    const std::string dictionaryId = "dictionary_id";

    dictionaryManager.updateDictionary( dictionaryId, JSON_EMPTY_OBJ );
    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );

    dictionaryManager.updateDictionary( dictionaryId, Json( "Test Json" ) );
    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

    dictionaryManager.updateDictionary( dictionaryId, JSON_EMPTY_OBJ );
    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( FakeDictionaryManagerTestSuite, readDictionary ) /* 1.3 */ {
    FakeDictionaryManager dictionaryManager;

    const std::string dictionaryId = "dictionary_id";
    auto testJson = Json( "Test Json" );
    dictionaryManager.updateDictionary( dictionaryId, testJson );

    auto dictionaryJson = dictionaryManager.readDictionary( dictionaryId );
    EXPECT_EQ( dictionaryJson.dump(), testJson.dump() );
};

TEST_F( FakeDictionaryManagerTestSuite, clearDictionary ) /* 1.4 */ {
    FakeDictionaryManager dictionaryManager;

    const std::string dictionaryId = "dictionary_id";
    dictionaryManager.updateDictionary( dictionaryId, Json( "Test Json" ) );
    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

    dictionaryManager.clearDictionary( dictionaryId );
    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

}  // namespace UT
}  // namespace HubPlatform
