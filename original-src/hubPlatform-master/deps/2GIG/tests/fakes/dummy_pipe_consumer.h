#pragma once

#include "utils/pipeline/pipe_consumer.h"

#include <vector>

namespace TwoGig {
namespace Test {

template < typename PayloadT >
class DummyPipeConsumer : public PipeConsumer< PayloadT > {
public:
    DummyPipeConsumer()
    : m_idx( 0 ) {}

    void processPayload( PayloadT payload ) override {
        m_payloadsQueue.emplace_back( std::move( payload ) );
    }

    const PayloadT* pullReceivedPayload() {
        if( m_idx == m_payloadsQueue.size() ) {
            return nullptr;
        }
        else {
            return &m_payloadsQueue[ m_idx++ ];
        }
    }

    void reset() {
        m_payloadsQueue.clear();
        m_idx = 0;
    }

private:
    std::vector< PayloadT > m_payloadsQueue;
    size_t m_idx;
};

}  // namespace Test
}  // namespace TwoGig
