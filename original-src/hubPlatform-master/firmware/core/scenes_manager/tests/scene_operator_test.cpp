#include "scenes_manager/scenes_engine/operators/operator_factory.h"
#include "scenes_manager/scenes_engine/operators/event_operator.h"
#include <gtest/gtest.h>
#include <utility>

namespace SceneEngine {
namespace UT {

/*

TESTPLAN

DONE    1. Scene Operator
DONE        1.0 Event operator
DONE        1.1 Not operator
DONE        1.2 And operator
DONE        1.3 Or operator
DONE        1.4 Nested operators
DONE            1.4.1 test 1
DONE            1.4.2 test 2
DONE            1.4.3 nested Not operators
DONE        1.5 Enable/disable operators
DONE            1.5.1 Event operator
DONE            1.5.2 Not operator
DONE            1.5.3 And operator
DONE            1.5.4 Or operator
DONE        1.6 Enable/disable nested operators
DONE            1.6.1 test 1
DONE            1.6.2 test 2
DONE        1.7 Xor operator

*/

class OperatorTests : public testing::Test {
public:
    static std::shared_ptr< EventOperator > createEventOperator( AbstractOperator::SharedOperator parent ) {
        auto eventOperator = std::make_shared< EventOperator >();
        eventOperator->setParent( std::move( parent ) );
        return eventOperator;
    }

    static AbstractOperator::SharedOperator createOrOperator( AbstractOperator::SharedOperator parent ) {
        auto operatorPtr = OperatorFactory::makeOrOperator();
        operatorPtr->setParent( std::move( parent ) );
        return operatorPtr;
    }

    static AbstractOperator::SharedOperator createAndOperator( AbstractOperator::SharedOperator parent ) {
        auto operatorPtr = OperatorFactory::makeAndOperator();
        operatorPtr->setParent( std::move( parent ) );
        return operatorPtr;
    }

    static AbstractOperator::SharedOperator createNotOperator( AbstractOperator::SharedOperator parent ) {
        auto operatorPtr = OperatorFactory::makeNotOperator();
        operatorPtr->setParent( std::move( parent ) );
        return operatorPtr;
    }

    static AbstractOperator::SharedOperator createXorOperator( AbstractOperator::SharedOperator parent ) {
        auto operatorPtr = OperatorFactory::makeXorOperator();
        operatorPtr->setParent( std::move( parent ) );
        return operatorPtr;
    }

    static bool evaluateSubtree( EventOperator& op, bool state ) {
        op.setState( state );
        const auto subtreeState = op.evaluateSubtree();
        return AbstractOperator::toBool( subtreeState );
    }
};

TEST_F( OperatorTests, eventOperator ) {  // 1.0
    auto eventOperator = std::make_shared< EventOperator >();
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
}

TEST_F( OperatorTests, notOperator ) {  // 1.1
    auto notOperator = createNotOperator( nullptr );
    auto eventOperator1 = createEventOperator( notOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, false ) );
    auto eventOperator2 = createEventOperator( notOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
}

TEST_F( OperatorTests, andOperator ) {  // 1.2
    auto andOperator = createAndOperator( nullptr );

    auto eventOperator1 = createEventOperator( andOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    auto eventOperator2 = createEventOperator( andOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    auto eventOperator3 = createEventOperator( andOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
}

TEST_F( OperatorTests, orOperator ) {  // 1.3
    auto orOperator = createOrOperator( nullptr );
    auto eventOperator1 = createEventOperator( orOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    auto eventOperator2 = createEventOperator( orOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    auto eventOperator3 = createEventOperator( orOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );

    auto eventOperator4 = createEventOperator( orOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, false ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, false ) );
}

TEST_F( OperatorTests, nestedOperators ) {  // 1.4.1
    auto andOperator0 = createAndOperator( nullptr );
    auto andOperator1 = createAndOperator( andOperator0 );
    auto andOperator2 = createAndOperator( andOperator0 );
    auto eventOperator1 = createEventOperator( andOperator1 );
    auto eventOperator2 = createEventOperator( andOperator1 );
    auto eventOperator3 = createEventOperator( andOperator2 );
    auto eventOperator4 = createEventOperator( andOperator2 );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );

    auto orOperator1 = createOrOperator( andOperator0 );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );

    auto eventOperator5 = createEventOperator( orOperator1 );
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, true ) );

    auto notOperator = createNotOperator( orOperator1 );
    auto eventOperator6 = createEventOperator( notOperator );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator6, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, true ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator5, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator4, true ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator4, false ) );
}

TEST_F( OperatorTests, nestedOperators1 ) {  // 1.4.2
    auto andOperator0 = createAndOperator( nullptr );
    auto andOperator1 = createAndOperator( andOperator0 );
    auto eventOperator1 = createEventOperator( andOperator1 );
    auto eventOperator2 = createEventOperator( andOperator1 );
    auto eventOperator3 = createEventOperator( andOperator0 );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
}

TEST_F( OperatorTests, nestedNotOperators ) {  // 1.4.3
    auto notOperator0 = createNotOperator( nullptr );
    auto notOperator1 = createNotOperator( notOperator0 );
    auto eventOperator1 = createEventOperator( notOperator1 );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
}

TEST_F( OperatorTests, enableEventOperator ) {  // 1.5.1
    auto eventOperator = createEventOperator( nullptr );
    eventOperator->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    eventOperator->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
}

TEST_F( OperatorTests, enableNotOperator ) {  // 1.5.2
    auto notOperator = createNotOperator( nullptr );
    notOperator->setEnabled( false );
    auto eventOperator = createEventOperator( notOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    notOperator->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
}

TEST_F( OperatorTests, enableAndOperator ) {  // 1.5.3
    auto andOperator = createAndOperator( nullptr );
    auto eventOperator1 = createEventOperator( andOperator );
    andOperator->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    auto eventOperator2 = createEventOperator( andOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );

    andOperator->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    eventOperator2->setEnabled( false );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    eventOperator1->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    eventOperator1->setEnabled( true );
    eventOperator2->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
}

TEST_F( OperatorTests, enableOrOperator ) {  // 1.5.4
    auto orOperator = createOrOperator( nullptr );
    orOperator->setEnabled( false );
    auto eventOperator1 = createEventOperator( orOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    auto eventOperator2 = createEventOperator( orOperator );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    orOperator->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    eventOperator1->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    eventOperator2->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );

    eventOperator1->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    eventOperator2->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
}

TEST_F( OperatorTests, enableNestedOperator1 ) {  // 1.6.1
    auto orOperator = createOrOperator( nullptr );
    auto eventOperator1 = createEventOperator( orOperator );
    auto notOperator = createNotOperator( orOperator );
    auto eventOperator2 = createEventOperator( notOperator );

    orOperator->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    orOperator->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    notOperator->setEnabled( false );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    notOperator->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );

    eventOperator2->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    eventOperator2->setEnabled( true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, false ) );

    eventOperator1->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
}

TEST_F( OperatorTests, enableNestedOperator2 ) {  // 1.6.1
    auto andOperator = createAndOperator( nullptr );
    auto eventOperator1 = createEventOperator( andOperator );
    auto notOperator = createNotOperator( andOperator );
    auto eventOperator2 = createEventOperator( notOperator );

    andOperator->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    andOperator->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );

    notOperator->setEnabled( false );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    notOperator->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );

    eventOperator2->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    eventOperator2->setEnabled( true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );

    eventOperator1->setEnabled( false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
}

TEST_F( OperatorTests, xorOperator ) {  // 1.7
    auto xorOperator = createXorOperator( nullptr );
    auto eventOperator1 = createEventOperator( xorOperator );

    // The implementation of xor operator with one EventOperator returns Disabled/false
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    /*   V1  V2  XOR
     *    0   0    0
     *    0   1    1
     *    1   1    0
     */
    auto eventOperator2 = createEventOperator( xorOperator );
    evaluateSubtree( *eventOperator1, false );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, true ) );

    evaluateSubtree( *eventOperator1, false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );

    evaluateSubtree( *eventOperator1, true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );

    evaluateSubtree( *eventOperator1, true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );

    /*   V1  V2  V3  XOR
     *    0   0   0    0
     *    0   0   1    1
     *    0   1   1    0
     *    1   1   1    1
     */
    auto eventOperator3 = createEventOperator( xorOperator );
    evaluateSubtree( *eventOperator1, false );
    evaluateSubtree( *eventOperator2, false );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );

    evaluateSubtree( *eventOperator1, false );
    evaluateSubtree( *eventOperator2, false );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );

    evaluateSubtree( *eventOperator1, false );
    evaluateSubtree( *eventOperator2, true );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );

    evaluateSubtree( *eventOperator1, true );
    evaluateSubtree( *eventOperator2, true );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );
}

}  // namespace UT
}  // namespace SceneEngine
