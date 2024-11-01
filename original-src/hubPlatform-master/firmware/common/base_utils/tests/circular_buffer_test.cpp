#include "base_utils/circular_buffer.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

template < typename T >
class CircularBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique< common::CircularBuffer< T > >( 3 );
    }

    std::unique_ptr< common::CircularBuffer< T > > buffer;

    std::vector< T > getTestData( int ) {
        return { T( 1 ), T( 2 ), T( 3 ), T( 4 ) };
    }

    std::vector< T > getTestData( double ) {
        return { T( 1.1 ), T( 2.2 ), T( 3.3 ), T( 4.4 ) };
    }

    std::vector< T > getTestData( std::string ) {
        return { "one", "two", "three", "four" };
    }
};

using MyTypes = ::testing::Types< int, double, std::string >;
TYPED_TEST_CASE( CircularBufferTest, MyTypes );

TYPED_TEST( CircularBufferTest, PutAndGetTest ) {
    for( size_t i = 0; i < 3; ++i ) {
        this->buffer->push( this->getTestData( TypeParam() )[ i ] );
    }

    for( size_t i = 0; i < 3; ++i ) {
        EXPECT_EQ( *this->buffer->pop(), this->getTestData( TypeParam() )[ i ] );
    }
    EXPECT_TRUE( this->buffer->empty() );
}

TYPED_TEST( CircularBufferTest, OverwriteTest ) {
    for( const auto& item : this->getTestData( TypeParam() ) ) {
        this->buffer->push( item );
    }

    for( size_t i = 1; i < 4; ++i ) {
        EXPECT_EQ( *this->buffer->pop(), this->getTestData( TypeParam() )[ i ] );
    }
    EXPECT_TRUE( this->buffer->empty() );
}

TYPED_TEST( CircularBufferTest, EmptyTest ) {
    EXPECT_TRUE( this->buffer->empty() );
    EXPECT_FALSE( this->buffer->pop().has_value() );
}

TYPED_TEST( CircularBufferTest, SizeAndCapacityTest ) {
    constexpr size_t capacity = 3;

    EXPECT_EQ( this->buffer->capacity(), capacity );
    EXPECT_EQ( this->buffer->size(), 0 );

    for( const auto& item : this->getTestData( TypeParam() ) ) {
        this->buffer->push( item );
    }

    EXPECT_EQ( this->buffer->size(), capacity );

    this->buffer->push( this->getTestData( TypeParam() )[ 3 ] );
    EXPECT_EQ( this->buffer->size(), capacity );
}

TYPED_TEST( CircularBufferTest, IsFullTest ) {
    EXPECT_FALSE( this->buffer->isFull() );

    for( const auto& item : this->getTestData( TypeParam() ) ) {
        this->buffer->push( item );
    }

    EXPECT_TRUE( this->buffer->isFull() );

    this->buffer->pop();
    EXPECT_FALSE( this->buffer->isFull() );
}

}  // namespace UT
}  // namespace HubPlatform
