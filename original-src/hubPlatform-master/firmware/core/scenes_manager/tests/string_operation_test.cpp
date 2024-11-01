#include "scenes_manager/scenes_engine/events/string_operation_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"

#include "test_utils/test_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

namespace Operations = Model::Resources::StringOperations;
using HubPlatform::Tests::replaceTokensInString;

/*

TEST PLAN

DONE    1. "begin" operation
DONE    2. "not_begin" operation
DONE    3. "end" operation
DONE    4. "not_end" operation
DONE    5. "contain" operation
DONE    6. "not_contain" operation
DONE    7. "length" operation
DONE    8. "not_length" operation
DONE    9. event is not created with incorrect operation
DONE    10. event is not created with incorrect operands
DONE        10.1 incorrect operand for "length" and "not_length"
DONE        10.2 incorrect operand for "begin", "not_begin", "end", "not_end", "contain", "not_contain"
DONE    11. Stateless operation
DONE        11.1 "contain" operation
DONE        11.2 "begin" operation

 */

class StringOperationEventTests : public testing::Test {
public:
    void checkExceptionOnEventCreation( Json valuePayload,
                                        const std::string& operation,
                                        const std::string& expectedMessage ) {
        bool exceptionCaught = false;
        try {
            ItemValueWrapper value( std::move( valuePayload ) );
            StringOperationEvent event( std::move( value ), operation );
        }
        catch( const SceneException& ex ) {
            exceptionCaught = true;
            EXPECT_EQ( ex.what(), expectedMessage );
        }
        EXPECT_TRUE( exceptionCaught );
    }

    std::string replaceOperation( const std::string& string, const std::string& operation ) {
        return replaceTokensInString( string, { { "__OPERATION__", operation } } );
    }
};

TEST_F( StringOperationEventTests, begin ) {  // 1.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::Begin );

    EXPECT_FALSE( event.compare( { "te" } ) );
    EXPECT_TRUE( event.compare( { "test_bigger_1" } ) );
    EXPECT_FALSE( event.compare( { "test_bigger_2" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );
    EXPECT_FALSE( event.compare( { "test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, notBegin ) {  // 2.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::NotBegin );

    EXPECT_TRUE( event.compare( { "te" } ) );
    EXPECT_FALSE( event.compare( { "te" } ) );

    EXPECT_FALSE( event.compare( { "test" } ) );
    EXPECT_TRUE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, end ) {  // 3.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::End );

    EXPECT_FALSE( event.compare( { "te" } ) );
    EXPECT_TRUE( event.compare( { "1_bigger_test" } ) );
    EXPECT_FALSE( event.compare( { "2_bigger_test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );
    EXPECT_FALSE( event.compare( { "test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, notEnd ) {  // 4.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::NotEnd );

    EXPECT_TRUE( event.compare( { "te" } ) );
    EXPECT_FALSE( event.compare( { "te" } ) );

    EXPECT_FALSE( event.compare( { "test" } ) );
    EXPECT_TRUE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, contain ) {  // 5.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::Contain );

    EXPECT_FALSE( event.compare( { "te" } ) );
    EXPECT_TRUE( event.compare( { "1_bigger_test" } ) );
    EXPECT_FALSE( event.compare( { "2_bigger_test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );
    EXPECT_FALSE( event.compare( { "test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, notContain ) {  // 6.
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::NotContain );

    EXPECT_TRUE( event.compare( { "te" } ) );
    EXPECT_FALSE( event.compare( { "1_bigger_test" } ) );
    EXPECT_FALSE( event.compare( { "2_bigger_test" } ) );

    EXPECT_TRUE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "test" } ) );

    EXPECT_TRUE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, length ) {  // 7.
    ItemValueWrapper value( Json{ { "value", 2 } } );
    StringOperationEvent event( value, Operations::Length );

    EXPECT_FALSE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "a" } ) );
    EXPECT_TRUE( event.compare( { "aa" } ) );
    EXPECT_FALSE( event.compare( { "aa" } ) );
    EXPECT_FALSE( event.compare( { "aaa" } ) );
    EXPECT_FALSE( event.compare( { "aaaa" } ) );
}

TEST_F( StringOperationEventTests, notLength ) {  // 8.
    ItemValueWrapper value( Json{ { "value", 2 } } );
    StringOperationEvent event( value, Operations::NotLength );

    EXPECT_TRUE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );

    EXPECT_FALSE( event.compare( { "aa" } ) );
    EXPECT_TRUE( event.compare( { "a" } ) );

    EXPECT_FALSE( event.compare( { "aa" } ) );
    EXPECT_TRUE( event.compare( { "aaa" } ) );
}

TEST_F( StringOperationEventTests, eventNotCreatedIncorrectOperation ) {  // 9.
    Json valuePayload{ { "value", 2 } };
    const std::string operation = "incorrect_operation";
    const std::string expectedMessage = "stringOperation: unknown operation: " + operation;
    checkExceptionOnEventCreation( std::move( valuePayload ), operation, expectedMessage );
}

TEST_F( StringOperationEventTests, eventNotCreatedIncorrectOperandForLength ) {  // 10.1
    Json value{ { "value", "2" } };
    const std::string error = "stringOperation: '__OPERATION__' operation requires operand to be a number";

    checkExceptionOnEventCreation( value, Operations::Length, replaceOperation( error, Operations::Length ) );
    checkExceptionOnEventCreation( value, Operations::NotLength, replaceOperation( error, Operations::NotLength ) );
}

TEST_F( StringOperationEventTests, eventNotCreatedIncorrectOperandWhereStringExpected ) {  // 10.2
    const std::string error = "stringOperation: '__OPERATION__' operation requires operand to be a non-empty string";

    {
        Json value{ { "value", 2 } };  // not a string
        checkExceptionOnEventCreation( value, Operations::Begin, replaceOperation( error, Operations::Begin ) );
        checkExceptionOnEventCreation( value, Operations::NotBegin, replaceOperation( error, Operations::NotBegin ) );
        checkExceptionOnEventCreation( value, Operations::End, replaceOperation( error, Operations::End ) );
        checkExceptionOnEventCreation( value, Operations::NotEnd, replaceOperation( error, Operations::NotEnd ) );
        checkExceptionOnEventCreation( value, Operations::Contain, replaceOperation( error, Operations::Contain ) );
        checkExceptionOnEventCreation(
            value, Operations::NotContain, replaceOperation( error, Operations::NotContain ) );
    }
    {
        Json value{ { "value", "" } };  // empty string
        checkExceptionOnEventCreation( value, Operations::Begin, replaceOperation( error, Operations::Begin ) );
        checkExceptionOnEventCreation( value, Operations::NotBegin, replaceOperation( error, Operations::NotBegin ) );
        checkExceptionOnEventCreation( value, Operations::End, replaceOperation( error, Operations::End ) );
        checkExceptionOnEventCreation( value, Operations::NotEnd, replaceOperation( error, Operations::NotEnd ) );
        checkExceptionOnEventCreation( value, Operations::Contain, replaceOperation( error, Operations::Contain ) );
        checkExceptionOnEventCreation(
            value, Operations::NotContain, replaceOperation( error, Operations::NotContain ) );
    }
}

TEST_F( StringOperationEventTests, containWithoutState ) {  // 11
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::Contain );
    event.setKeepState( false );
    EXPECT_FALSE( event.compare( { "te" } ) );
    EXPECT_TRUE( event.compare( { "1_bigger_test" } ) );
    EXPECT_TRUE( event.compare( { "2_bigger_test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

TEST_F( StringOperationEventTests, beginWithoutState ) {  // 11
    ItemValueWrapper value( Json{ { "value", "test" } } );
    StringOperationEvent event( value, Operations::Begin );
    event.setKeepState( false );

    EXPECT_FALSE( event.compare( { "te" } ) );
    EXPECT_TRUE( event.compare( { "test_bigger_1" } ) );
    EXPECT_TRUE( event.compare( { "test_bigger_2" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );
    EXPECT_TRUE( event.compare( { "test" } ) );

    EXPECT_FALSE( event.compare( { "reset" } ) );
    EXPECT_FALSE( event.compare( { "" } ) );
}

}  // namespace UT
}  // namespace SceneEngine
