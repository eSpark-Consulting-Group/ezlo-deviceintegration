#include "base_utils/spi/spi_transport.h"

#include <cstring>
#include <fstream>
#include <unistd.h>

int32_t SpiTransport::init( uint32_t spiSpeed, const bool keepCSActivated ) {
    /*__u8*/ uint8_t lsb, bits = 8;
    /*__u32*/ uint32_t speed = spiSpeed;
    /*__u32*/ uint32_t mode = SPI_MODE_0;
    int32_t spi = open( spiDeviceFilename, O_RDWR );
    if( spi < 0 ) {
        std::cerr << "Failed to open the spi bus " << spiDeviceFilename << std::endl;
        exit( EXIT_FAILURE );
    }
    if( ioctl( spi, SPI_IOC_RD_MODE, &mode ) < 0 ) {
        perror( "SPI rd_mode" );
        std::cerr << "Failed to init the spi bus: SPI rd_mode" << std::endl;
        exit( EXIT_FAILURE );
    }
    if( ioctl( spi, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 ) {
        perror( "SPI rd_lsb_fist" );
        std::cerr << "Failed to init the spi bus: SPI rd_lsb_fist" << std::endl;
        exit( EXIT_FAILURE );
    }
    if( ioctl( spi, SPI_IOC_WR_BITS_PER_WORD, &bits ) < 0 ) {
        perror( "SPI bits_per_word" );
        std::cerr << "Failed to init the spi bus: SPI bits_per_word" << std::endl;
        exit( EXIT_FAILURE );
    }
    if( ioctl( spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed ) < 0 ) {
        perror( "Failed to set max speed hz" );
        std::cerr << "Failed to init the spi bus: set SPI_IOC_WR_MAX_SPEED_HZ" << std::endl;
        exit( EXIT_FAILURE );
    }
    if( ioctl( spi, SPI_IOC_RD_MAX_SPEED_HZ, &speed ) < 0 ) {
        perror( "SPI max_speed_hz" );
        std::cerr << "Failed to init the spi bus: set SPI_IOC_RD_MAX_SPEED_HZ" << std::endl;
        exit( EXIT_FAILURE );
    }
    std::memset( &m_xfer[ 0 ], 0, sizeof( spi_ioc_transfer ) );
    static_assert( sizeof( m_xfer[ 0 ].tx_buf ) >= sizeof( size_t ),
                   "spi_ioc_transfer::tx(rx)_buf and size_t size mismatch !" );
    m_xfer[ 0 ].tx_buf = reinterpret_cast< size_t >( m_txBuffer );
    m_xfer[ 0 ].len = 4;                                  // Length of  command to write
    m_xfer[ 0 ].cs_change = ( keepCSActivated ? 0 : 1 );  // Keep CS activated
    m_xfer[ 0 ].delay_usecs = 200;                        // Delay in (us)
    m_xfer[ 0 ].speed_hz = speed;                         // Speed (Hz)
    m_xfer[ 0 ].bits_per_word = 8;                        // Bits per word

    m_descriptor = spi;
    m_isValid = true;

    return m_descriptor;
}

void SpiTransport::deinit() {
    close( m_descriptor );
    m_descriptor = -1;
    m_isValid = false;
}

int SpiTransport::sendRequest( const std::vector< uint8_t >& command,
                               int32_t resultSize,
                               std::vector< uint8_t >& outResult ) {
    memset( m_txBuffer, 0, sizeof( m_txBuffer ) );
    memset( m_rxBuffer, 0, sizeof( m_rxBuffer ) );
    for( int i = 0; i < command.size(); ++i ) {
        m_txBuffer[ i ] = command[ i ];
    }
    static_assert( sizeof( m_xfer[ 0 ].tx_buf ) >= sizeof( size_t ),
                   "spi_ioc_transfer::tx(rx)_buf and size_t size mismatch !" );
    m_xfer[ 0 ].tx_buf = reinterpret_cast< size_t >( m_txBuffer );
    m_xfer[ 0 ].rx_buf = reinterpret_cast< size_t >( m_rxBuffer );

    m_xfer[ 0 ].len = command.size() + resultSize;
    outResult.clear();
    int status = ioctl( m_descriptor, SPI_IOC_MESSAGE( 1 ), m_xfer );
    if( status < 0 ) {
        perror( "SPI_IOC_MESSAGE" );
        return 0;
    }

#if( DEBUG_TRANSPORT == 1 )
    printf( "TX: " );
    for( int i = 0; i < ( command.size() + resultSize ); ++i ) {
        printf( "%02x ", m_txBuffer[ i ] );
    }
    printf( "\n" );
    printf( "RX: " );
    for( int i = command.size(); i < ( command.size() + resultSize ); ++i ) {
        printf( "%02x ", m_rxBuffer[ i ] );
    }
    printf( "\n" );
#endif

    if( resultSize == 0 ) {
        outResult.resize( command.size() );
        std::copy( m_rxBuffer, m_rxBuffer + command.size(), outResult.begin() );
    }
    else {
        outResult.resize( resultSize );
        std::copy( &m_rxBuffer[ command.size() ], &m_rxBuffer[ command.size() ] + resultSize, outResult.begin() );
    }

    return outResult.size();
}

bool SpiTransport::isValid() const {
    return m_isValid;
}
