#include "i2ccontrol.h"

#include <i2c/i2c-dev.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace i2c {
I2cControl::I2cControl( const std::string& devPath ) {
    devDescriptor_ = init( devPath );
    if( devDescriptor_ < 0 )
        std::cerr << "Failed to open device by address " + devPath << "\n";
}

I2cControl::~I2cControl() {
    close( devDescriptor_ );
}

bool I2cControl::write( Byte command, const ByteData& data ) {
    struct i2c_rdwr_ioctl_data i2c;  // structure pass to i2c driver
    struct i2c_msg i2cmsg[ 1 ];      // message count in the structure
    i2c.nmsgs = 1;                   // message count in the structure
    i2c.msgs = i2cmsg;
    i2c.msgs[ 0 ].len = data.size();
    i2c.msgs[ 0 ].addr = command;
    i2c.msgs[ 0 ].flags = 0;  // 0 for write;
    i2c.msgs[ 0 ].buf = reinterpret_cast< char* >( const_cast< Byte* >( data.data() ) );
    int result = ioctl( devDescriptor_, I2C_RDWR, (unsigned long) &i2c );
    return result >= 0;
}

ByteData I2cControl::read( Byte command, const Byte msb, const Byte lsb, const Byte size ) {
    struct i2c_rdwr_ioctl_data i2c;  // structure pass to i2c driver
    struct i2c_msg i2cmsg[ 2 ];      // message count in the structure
    ByteData txBuf = { msb, lsb, 0x02 };
    ByteData buffer( size + 1 );
    i2c.nmsgs = 2;  // message count in the structure
    i2c.msgs = i2cmsg;
    i2c.msgs[ 0 ].len = 3;  // the offset address word is 2 bytes long
    i2c.msgs[ 0 ].addr = command;
    i2c.msgs[ 0 ].flags = 0;  // 0 for write;
    i2c.msgs[ 0 ].buf = reinterpret_cast< char* >( txBuf.data() );
    i2c.msgs[ 1 ].len = size;
    i2c.msgs[ 1 ].addr = 0xb1 >> 1;
    i2c.msgs[ 1 ].flags = I2C_M_RD;  // 1 for read;
    i2c.msgs[ 1 ].buf = reinterpret_cast< char* >( buffer.data() );
    if( ioctl( devDescriptor_, I2C_RDWR, (unsigned long) &i2c ) < 0 ) {
        return {};
    }
    buffer.resize( size );
    return buffer;
}

int I2cControl::init( const std::string& path ) {
    int descriptor = open( path.c_str(), O_RDWR );
    if( descriptor < 0 ) {
        std::cerr << "Failed to open I2C device " << path << "!\n";
        return -1;
    }
    ioctl( descriptor, I2C_TIMEOUT, 3 );  // i2c timeout, 1 for 10ms ; if too small, i2c may lose response
    ioctl( descriptor, I2C_RETRIES, 3 );  // i2c retry limit
    return descriptor;
}
}  // namespace i2c
