#include "utils/logger/logger.h"

#include "fakes/dummy_pipe_producer.h"
#include "fakes/dummy_pipe_consumer.h"
#include "fakes/fake_timer.h"

#include <gtest/gtest.h>

namespace TwoGig {
namespace Test {

template < typename PipeT >
class PipeTest : public testing::Test {
public:
    template < typename... ArgsT >
    PipeTest( ArgsT&&... args )
    : m_producer( m_logger )
    , m_pipe( m_logger, std::forward< ArgsT >( args )... ) {
        m_producer.pipe( m_pipe );

        for( size_t i = 0; i != PipeT::channelsCount; ++i ) {
            m_pipe.pipe( m_consumers[ i ], i );
        }
    }

    void SetUp() override {
        m_pipe.reset();

        resetConsumers();
    }

    void resetConsumers() {
        for( auto& consumer : m_consumers ) {
            consumer.reset();
        }
    }

    void pushPayload( typename PipeT::ConsumingPayload payload ) {
        m_producer.dispatchPayload( payload );
    }

    const typename PipeT::ProducingPayload* pullPayload( size_t pipeChannel = 0 ) {
        return m_consumers[ pipeChannel ].pullReceivedPayload();
    }

private:
    Logger m_logger;

    DummyPipeProducer< typename PipeT::ConsumingPayload > m_producer;
    PipeT m_pipe;
    std::array< DummyPipeConsumer< typename PipeT::ProducingPayload >, PipeT::channelsCount > m_consumers;
};

}  // namespace Test
}  // namespace TwoGig
