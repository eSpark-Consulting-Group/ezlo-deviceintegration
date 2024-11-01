#include "utils/serial_port/serial_port_reader.h"
#include "utils/serial_port/serial_port.h"

namespace TwoGig {

SerialPortReader::SerialPortReader( const Logger& logger, SerialPort& port )
: PipeProducer< uint8_t >( logger )
, m_port( port )
, m_isReading( false ) {}

SerialPortReader::~SerialPortReader() {
    stopReading();
}

void SerialPortReader::startReading() {
    if( m_isReading )
        return;

    if( !m_port.open() ) {
        m_logger.error( "Failed to open the port" );
        return;
    }

    m_isReading = true;

    m_producer = std::thread( [this] {
        uint8_t byte;
        while( m_isReading ) {
            if( m_port.readByte( byte ) )
                m_buffer.push( byte );
        }
    } );

    m_consumer = std::thread( [this] {
        uint8_t byte;
        while( m_isReading ) {
            if( m_buffer.pop( byte ) )
                dispatchPayload( byte );
        }
    } );
}

void SerialPortReader::stopReading() {
    m_isReading = false;

    if( m_producer.joinable() )
        m_producer.join();

    m_buffer.clear();
    m_buffer.unblockConsumer();

    if( m_consumer.joinable() )
        m_consumer.join();

    m_port.close();
}

}  // namespace TwoGig
