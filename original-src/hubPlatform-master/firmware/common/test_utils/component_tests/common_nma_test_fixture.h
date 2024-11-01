#pragma once

#include "test_utils/component_tests/component_test_application.h"

#include "json_utils/json_fwd.h"

#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <set>
#include <utility>

namespace HubPlatform {
namespace Tests {

static const std::chrono::seconds BroadcastTimeout( 30 );
static const std::chrono::seconds progressBroadcastTimeout( 10 );
static const std::chrono::seconds avoidedBroadcastTimeout( 5 );
static const std::chrono::seconds ignoreBroadcastTimeout( 5 );
static const std::chrono::seconds IgnoreBroadcastsPeriod( 1 );

class CommonNmaTests : public testing::Test {
public:
    using Predicate = std::function< bool( const JsonObject& ) >;
    using KeyJsonList = std::vector< std::pair< std::string, JsonObject > >;
    using KeyStringList = std::vector< std::pair< std::string, std::string > >;
    using BroadcastName = std::string;
    using BroadcastChecker = std::function< void( const JsonObject& broadcast, int codeLine ) >;
    using BroadcastCheckers = std::map< BroadcastName, BroadcastChecker >;
    using BroadcastCheckerVector = std::vector< std::function< bool( const JsonObject& broadcast ) > >;

public:
    void SetUp() override;

    void TearDown() override;

    std::string getCodeLineString( int codeLine ) const;
    void callingFile( std::string value );

    JsonObject waitForBroadcast( const std::string& messageSubclass,
                                 const char* file,
                                 int line,
                                 std::chrono::milliseconds timeout = BroadcastTimeout );
    JsonObject waitForBroadcast( const std::string& messageSubclass,
                                 const std::string& comment = "",
                                 std::chrono::milliseconds timeout = BroadcastTimeout );

    void checkNoBroadcastReceived( const std::string& comment,
                                   int codeLine,
                                   std::chrono::milliseconds timeout = avoidedBroadcastTimeout );

    JsonObject waitForBroadcast( Predicate predicate,
                                 const std::string& comment = "",
                                 std::chrono::milliseconds timeout = ignoreBroadcastTimeout );

    JsonObject waitForAnyOfBroadcasts( std::set< std::string > messageSubclassSet,
                                       const std::string& comment = "",
                                       std::chrono::milliseconds timeout = ignoreBroadcastTimeout );

    void ignoreBroadcast( Predicate predicate, std::chrono::seconds timeout = ignoreBroadcastTimeout );
    void ignoreBroadcasts( Predicate predicate,
                           const char* file = "",
                           int line = 0,
                           std::chrono::seconds period = IgnoreBroadcastsPeriod );
    void ignoreBroadcasts( Predicate predicate, const char* file, int line, std::chrono::milliseconds period );

    Services::Message waitForInternalBroadcast( const char* file = "",
                                                int line = 0,
                                                std::chrono::seconds timeout = BroadcastTimeout );

    void checkNoInternalBroadcastReceived( const char* file = "",
                                           int line = 0,
                                           std::chrono::seconds timeout = avoidedBroadcastTimeout );

    static JsonObject replaceTokens( const JsonObject& json, const KeyStringList& replaceList );

    static JsonObject replaceTokens( std::string json, const KeyStringList& replaceList );

    static std::string replaceTokensInString( std::string str, const KeyStringList& replaceList );
    static std::string replaceTokensInStringNoFail( std::string str, const KeyStringList& replaceList );

    static void compareBroadcastFields( const Json& broadcast,
                                        const std::string& id,
                                        const std::string& broadcastName,
                                        const KeyJsonList& fields,
                                        const std::string& message );

    static bool compareBroadcastAndLookingForFields( const Json& broadcast,
                                                     const std::string& id,
                                                     const std::string& broadcastName,
                                                     const KeyJsonList& fields,
                                                     const std::string& message );

    void checkBroadcast( const std::string& id,
                         const std::string& broadcastName,
                         const KeyJsonList& fields,
                         int codeLine );
    void checkBroadcast( const std::string& id,
                         const std::string& broadcastName,
                         const KeyJsonList& fields,
                         const std::string& message );

    void checkBroadcasts( const BroadcastCheckers& broadcasts, int codeLine );
    void checkBroadcasts( const BroadcastCheckerVector& broadcastCheckers, int codeLine );

    void sendRequest( const Json& request, int codeLine );

    static ComponentTestApplication& nma();

    static Json safeParseJson( const char* file, int line, const std::string& jsonStr );

    static std::string getBigString() {
        std::string data;
        for( int i = 0; i < 20000; i++ ) {
            data += std::to_string( i );
            data += " ";
        }

        return data;
    }
    void updatePredefineGroupsForTest( const std::string& entryDelayValue );

protected:
    void assertTrue( bool condition, const std::string& comment );
    const std::string& getCallingFile() const;

private:
    std::mutex m_broadcastsMutex;
    std::condition_variable m_broadcastsCondVar;
    std::deque< JsonObject > m_broadcastMessages;

    std::mutex m_internalBroadcastsMutex;
    std::condition_variable m_internalBroadcastsCondVar;
    std::deque< Services::Message > m_internalBroadcastMessages;

    std::string m_callingFile;  //!< file name prefix for getCodeLineString(); set in every test
};

#define NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST() waitForInternalBroadcast( __FILE__, __LINE__ )
#define NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS() checkNoInternalBroadcastReceived( __FILE__, __LINE__ )
#define NMA_TEST_WAIT_FOR_BROADCAST( messageSubclass ) waitForBroadcast( messageSubclass, __FILE__, __LINE__ )
#define NMA_TEST_WAIT_NO_BROADCASTS() \
    checkNoBroadcastReceived(         \
        std::string( __FILE__ ) + ":" + std::to_string( __LINE__ ) + " - not expected broadcast", __LINE__ )

#define NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( messageSubclass ) \
    ( {                                                               \
        auto br = NMA_TEST_WAIT_FOR_BROADCAST( messageSubclass );     \
        br.erase( "msg_id" );                                         \
        br;                                                           \
    } )

#define PARSE_JSON( jsonStr ) safeParseJson( __FILE__, __LINE__, jsonStr )

#define NMA_TEST_MAKE_MESSAGE( msg ) \
    std::string{ __FILE__ }.append( ":" + std::to_string( __LINE__ ) ).append( ", message: " ).append( ( msg ) )

namespace Resources {
namespace CheckBroadcast {
constexpr char Has[] = "$has";
constexpr char HasNot[] = "$has-not";
}  // namespace CheckBroadcast
}  // namespace Resources

}  // namespace Tests
}  // namespace HubPlatform
