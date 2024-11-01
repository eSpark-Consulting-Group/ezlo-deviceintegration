#pragma once

#include <string>
#include <vector>

namespace i2c {
using Byte = unsigned char;
using ByteData = std::vector< Byte >;

class I2cControl final {
public:
    I2cControl( const std::string& devPath );
    ~I2cControl();
    bool write( Byte command, const ByteData& data );
    ByteData read( Byte command, const Byte msb, const Byte lsb, const Byte size );

private:
    int init( const std::string& path );

    int devDescriptor_;
};

}  // namespace i2c
