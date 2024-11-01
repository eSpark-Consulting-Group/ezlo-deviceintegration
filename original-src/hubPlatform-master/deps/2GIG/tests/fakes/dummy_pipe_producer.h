#pragma once

#include "utils/pipeline/pipe_producer.h"

namespace TwoGig {
namespace Test {

template < typename PayloadT >
class DummyPipeProducer : public PipeProducer< PayloadT > {
public:
    DummyPipeProducer( const Logger& logger )
    : PipeProducer< PayloadT >( logger ) {}

    using PipeProducer< PayloadT >::dispatchPayload;
};

}  // namespace Test
}  // namespace TwoGig
