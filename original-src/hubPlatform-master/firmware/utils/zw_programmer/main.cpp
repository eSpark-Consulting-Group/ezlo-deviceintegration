#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <chrono>
#include <ctime>

#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"

#include "ha_719.hex"
#include "ha_799.hex"
#include "ha_718.hex"

using namespace std;

static int serial_fd;

long double curtime() {
    return std::chrono::duration_cast< std::chrono::milliseconds >(
               std::chrono::system_clock::now().time_since_epoch() )
        .count();
}

int serial_open( char* serial_name ) {
    int fd;
    fd = open( serial_name, O_RDWR | O_NOCTTY );
    /* 8N1 Mode */
    struct termios SerialPortSettings; /* Create the structure                          */

    tcgetattr( fd, &SerialPortSettings ); /* Get the current attributes of the Serial port */

    /* Setting the Baud rate */
    cfsetispeed( &SerialPortSettings, B115200 ); /* Set Read  Speed as 115200                       */
    cfsetospeed( &SerialPortSettings, B115200 ); /* Set Write Speed as 115200                       */

    /* 8N1 Mode */
    SerialPortSettings.c_cflag &= ~PARENB; /* Disables the Parity Enable bit(PARENB),So No Parity   */
    SerialPortSettings.c_cflag &= ~CSTOPB; /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    SerialPortSettings.c_cflag &= ~CSIZE;  /* Clears the mask for setting the data size             */
    SerialPortSettings.c_cflag |= CS8;     /* Set the data bits = 8                                 */

    SerialPortSettings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
    SerialPortSettings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */

    SerialPortSettings.c_iflag &= ~( IXON | IXOFF | IXANY ); /* Disable XON/XOFF flow control both i/p and o/p */
    SerialPortSettings.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG ); /* Non Cannonical mode */

    SerialPortSettings.c_oflag &= ~OPOST; /*No Output Processing*/

    /* Setting Time outs */
    SerialPortSettings.c_cc[ VMIN ] = 0;  /* Read at least 10 characters */
    SerialPortSettings.c_cc[ VTIME ] = 0; /* Wait indefinetly   */

    if( ( tcsetattr( fd, TCSANOW, &SerialPortSettings ) ) != 0 ) /* Set the attributes to the termios structure*/
        printf( "\n  ERROR ! in Setting attributes" );
    else
        printf( "\n  BaudRate = 115200 \n  StopBits = 1 \n  Parity   = none" );

    return fd;
}
void serial_send( char* data, int size ) {
    write( serial_fd, data, size );
}

int serial_read( char* data, int size, int timeout_usec ) {
    int count = 0;
    int n;
    auto start = curtime();
    do {
        n = read( serial_fd, &data[ count ], size - count );
        if( n >= 0 ) {
            count += n;
            data[ count ] = 0;
        }
        if( ( curtime() - start ) >= timeout_usec )
            return count;
    } while( count < size );
    return count;
}

void serial_close( int fd ) {
    close( fd );
}

static uint16_t calculateCRC( unsigned char* ptr, int count ) {
    uint16_t crc;
    char i;
    crc = 0;
    while( --count >= 0 ) {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do {
            if( crc & 0x8000 )
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while( --i );
    }
    return ( crc );
}

int Inbyte( unsigned short t ) {
    char data[ 2 ];
    int n = serial_read( data, 1, t );
    if( n > 0 ) {
        return data[ 0 ];
    }
    return -1;
}

void Outbyte( int c ) {
    uint8_t ch;
    if( c >= 0 ) {
        ch = c & 0xff;
        write( serial_fd, &ch, 1 );
    }
}

static int XmodemTransmit( uint8_t* data_f, int len ) {
    uint8_t data[ 240 ];
    int cur_p = 0;
    uint8_t pack = 1;
    int old_d = len;
    int cur_pk = 0;
    bool is_C = false;
    auto start = curtime();
    while( curtime() - start < 30000 ) {
        int c = Inbyte( 1500 );
        if( c == 'C' ) {
            cout << "Connect OK!" << endl;
            usleep( 300000 );
            is_C = true;
            break;
        }
    }
    if( !is_C ) {
        cout << "XMODEM TIME OUT!" << endl;
        return -1;
    }

start_xm:

    while( 1 ) {
        memset( &data, 0, 133 );
        data[ 0 ] = SOH;
        data[ 1 ] = pack;
        data[ 2 ] = 0xff - pack;

        memcpy( &data[ 3 ], &data_f[ cur_p ], old_d > 128 ? 128 : old_d );
        int crc = calculateCRC( &data[ 3 ], 128 );
        data[ 131 ] = ( crc >> 8 ) & 0xFF;
        data[ 132 ] = crc & 0xFF;
        for( int i = 0; i < 133; i++ ) {
            Outbyte( data[ i ] );
        }

        start = curtime();
        while( curtime() - start < 3500 ) {
            int c = Inbyte( TIME_INBYTE );
            if( c > 0 ) {
                if( c == ACK ) {
                    cout << "Send pak " << cur_pk << endl;
                    cur_pk++;
                    goto ack_ok;
                }
                if( c == NAK ) {
                    cout << "XMODEM NAK!" << endl;
                    goto start_xm;
                }
                if( c == CAN ) {
                    cout << "XMODEM Connection error!" << endl;
                    int n = serial_read( (char*) &data[ 0 ], 240, TIME_TO_READ );
                    for( int i = 0; i < n; i++ ) {
                        if( data[ i ] == 0 )
                            data[ i ] = 1;
                    }
                    cout << (char*) data << endl;
                    Outbyte( ACK );
                    return 0x18;
                }
            }
        }
        cout << "XMODEM ACK TIME OUT!" << endl;
        return -2;
    ack_ok:
        cur_p = cur_p + 128;
        old_d = len - cur_p;
        pack++;
        if( old_d <= 0 ) {
            Outbyte( EOT );
            start = curtime();
            while( curtime() - start < 3500 ) {
                int c = Inbyte( TIME_INBYTE );
                if( c > 0 ) {
                    if( c == ACK ) {
                        cout << "SEND OK!" << endl;
                        return len;
                    }
                    else if( c == NAK ) {
                        cout << "XMODEM NAK!" << endl;
                        return -3;
                    }
                    else if( c == CAN ) {
                        cout << "XMODEM Connection error wait ACK!" << endl;
                        int n = serial_read( (char*) &data[ 0 ], 240, TIME_TO_READ );
                        for( int i = 0; i < n; i++ ) {
                            if( data[ i ] == 0 )
                                data[ i ] = 1;
                        }
                        cout << (char*) data << endl;
                        Outbyte( ACK );

                        Outbyte( ACK );
                        return 0x18;
                    }
                    else {
                        char c1 = c;
                        cout << c1;
                    }
                }
            }
            cout << "XMODEM EOT non ACK!" << endl;
            return -5;
        }
    }
    return 0;
}

uint8_t is_booting() {
    char data1[ MAX_BUFF_LEN ];
    int n = serial_read( data1, MIN_BUFF_LEN, TIME_TO_READ );

    for( int i = 0; i < n; i++ ) {
        if( data1[ i ] == 0 )
            data1[ i ] = 1;
    }
    char* dst = strstr( data1, "Bootloader" );
    if( dst == NULL ) {
        cout << "NOT BOOT" << endl;
        serial_close( serial_fd );
        return 1;
    }
    else {
        cout << "BOOT OK!" << endl;
    }
    serial_read( data1, MAX_BUFF_LEN, TIME_TO_READ );
    printf( "%s\n", data1 );
    return 0;
}

int main( int argc, char* argv[] ) {
    auto start = curtime();
    cout << "ZW PRG START!" << start << endl;
    int n;

    static uint8_t device[] = { 0x01, 0x03, 0x00, 0x27, 0xdb };   // start bootloader for zwave 700
    static uint8_t get_ver[] = { 0x01, 0x03, 0x00, 0x07, 0xfb };  // get SF ver zwave 700
    static char data[ MAX_BUFF_LEN ];
    uint8_t ver1 = 0, ver2 = 0;

    if( argc < 2 ) {
        cout << "arg error " << endl;
        cout << "Help:\n\r -h or -help - this help;\n\r zw_programmer param1 param2 \n\r   param1 - port name (exampl: "
                "/dev/ttyUSB0) \n\r   param2 - file name FW\n\r"
             << "Example:\n\r   zw_programmer /dev/ttyUSB0 z14_7_20_2.gbl" << endl;
        return ERR_ARG;
    }

    char* dst1 = strstr( argv[ 1 ], "-h" );

    if( dst1 != NULL ) {
        cout << "Help:\n\r -h or -help - this help;\n\r zw_programmer param1 param2 \n\r   param1 - port name (exampl: "
                "/dev/ttyUSB0) \n\r   param2 - file name FW\n\r"
             << "Example:\n\r   zw_programmer /dev/ttyUSB0 z14_7_20_2.gbl" << endl;
        return ERR_OK;
    }

    char* portName = argv[ 1 ];
    char* filePath = argv[ 2 ];
    bool is_hack = false;

    serial_fd = serial_open( portName );

    if( serial_fd == -1 ) {
        printf( "Error opening the serial device: %s\n", argv[ 1 ] );
        return ERR_OPEN_PORT;
    }
    printf( "\nSERIAL OPEN:\n" );

    ifstream is;

    is.open( filePath, ios::in | ios::binary );
    // Check if the file has opened
    if( !is ) {
        cout << "Error while opening the file!" << endl;
        serial_close( serial_fd );
        return ERR_OPEN_FILE;
    }

    write( serial_fd, (char*) &get_ver[ 0 ], 5 );
    n = serial_read( data, MIN_BUFF_LEN, TIME_TO_READ );
    char* dst = strstr( data, "Bootloader" );
    is_hack = true;
    if( dst != NULL ) {
        is_hack = false;
        serial_read( data, MAX_BUFF_LEN, TIME_TO_READ );
    }
    else {
        for( int i = 0; i < n - 7; i++ ) {
            if( data[ i ] == 0x06 ) {
                if( data[ i + 4 ] == 0x07 ) {
                    ver1 = data[ i + 5 ];
                    ver2 = data[ i + 6 ];
                    printf( "SW ver. %d.%d\n\r", ver1, ver2 );
                    write( serial_fd, (char*) &data[ i ], 1 );
                    break;
                }
            }
        }
    }

    if( is_hack ) {
        serial_read( data, MAX_BUFF_LEN, TIME_TO_READ );
        write( serial_fd, (char*) &device[ 0 ], 5 );

        //  start_boot:
        if( is_booting() ) {
            return ERR_NO_BOOT;
        }
    }
    auto end_t = curtime() - start;
    cout << "TN ms " << end_t << endl;

    data[ 0 ] = '1';
    write( serial_fd, "1", 1 );

    uint8_t* buffer = (uint8_t*) &data_h_719[ 0 ];
    if( ver2 > 19 ) {
        buffer = (uint8_t*) &data_h_799[ 0 ];
    }
    while( Inbyte( TIME_INBYTE ) > 0 )
        ;

    write( serial_fd, (const void*) '1', 1 );
    cout << "Start XModem!" << endl;
    XmodemTransmit( &buffer[ 0 ], 2048 );
    while( Inbyte( TIME_INBYTE ) > 0 )
        ;

    Outbyte( '3' );

    int length;
    // get length of file:
    is.seekg( 0, ios::end );
    length = is.tellg();
    is.seekg( 0, ios::beg );
    // allocate memory:
    buffer = new unsigned char[ length ];
    // read data as a block:
    is.read( (char*) buffer, length );
    is.close();
    cout << "File len " << length << endl;
    if( is_booting() ) {
        return ERR_NO_BOOT;
    }
    Outbyte( '1' );

    int res = XmodemTransmit( &buffer[ 0 ], length );
    free( buffer );
    Outbyte( '2' );
    if( !is_booting() && res == length ) {
        serial_close( serial_fd );
        return ERR_OK;
    }

    Outbyte( '3' );
    cout << "Err upload gbl. Start upload 7_18 " << endl;
    if( is_booting() ) {
        return ERR_NO_BOOT;
    }
    Outbyte( '1' );

    res = XmodemTransmit( (unsigned char*) &data_h_718[ 0 ], len_718 );
    Outbyte( '2' );
    serial_close( serial_fd );
    if( res != len_718 ) {
        cout << "Err upload 7_18 " << endl;
        return ERR_XMODEM;
    }

    return ERR_SDK_718;
}
