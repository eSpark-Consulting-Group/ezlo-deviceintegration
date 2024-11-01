#pragma once

namespace TwoGig {

template < typename PayloadT >
struct PipeConsumer {
    using ConsumingPayload = PayloadT;

public:
    virtual ~PipeConsumer() = default;

    virtual void processPayload( PayloadT packet ) = 0;

    virtual void reset() {}
};

}  // namespace TwoGig
