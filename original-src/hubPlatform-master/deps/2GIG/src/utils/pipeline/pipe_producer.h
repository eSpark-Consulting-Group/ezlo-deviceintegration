#pragma once

#include "utils/pipeline/pipe_consumer.h"
#include "utils/logger/logger.h"

#include <cstdio>

namespace TwoGig {

template < typename PayloadT, size_t ChannelsCount = 1 >
struct PipeProducer {
    using ProducingPayload = PayloadT;

    static constexpr size_t channelsCount = ChannelsCount;

public:
    PipeProducer( const Logger& logger )
    : m_logger( logger )
    , m_pipes{ nullptr } {}

    template < typename T >
    T& pipe( T& pipe, size_t channel = 0 ) {
        m_pipes[ channel ] = &pipe;
        return pipe;
    }

    void resetPipe( size_t channel = 0 ) {
        m_pipes[ channel ] = nullptr;
    }

protected:
    void dispatchPayload( const PayloadT& packet, size_t channel = 0 ) const {
        if( auto pipe = m_pipes[ channel ] )
            pipe->processPayload( packet );
        else
            m_logger.warn( "Ignoring packet" );
    }

protected:
    const Logger& m_logger;

private:
    PipeConsumer< PayloadT >* m_pipes[ channelsCount ];
};

}  // namespace TwoGig
