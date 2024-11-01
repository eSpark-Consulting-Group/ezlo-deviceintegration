#pragma once

#include "utils/pipeline/pipe_producer.h"
#include "utils/thread/blocking_queue.h"

#include <functional>
#include <atomic>
#include <thread>

namespace TwoGig {

class SerialPort;

class SerialPortReader : public PipeProducer< uint8_t > {
public:
    SerialPortReader( const Logger& logger, SerialPort& port );

    ~SerialPortReader();

    void startReading();
    void stopReading();

private:
    SerialPort& m_port;

    BlockingQueue< uint8_t > m_buffer;

    std::atomic_bool m_isReading;
    std::thread m_consumer;
    std::thread m_producer;
};

}  // namespace TwoGig
