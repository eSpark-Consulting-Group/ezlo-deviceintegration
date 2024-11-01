#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <memory>

#include "base_utils/hubtypes.h"
#include "base_utils/i2ccontrol.h"

#if defined( __HUB_TYPE_A20__ )
static constexpr char AT88SC_i2cdevice[] = "/dev/i2c-1";
#elif defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ )
static constexpr char AT88SC_i2cdevice[] = "/dev/null";
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ ) || defined( __HUB_TYPE_G150__ ) || \
    defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
static constexpr char AT88SC_i2cdevice[] = "/dev/null";
#else
#error "Specify valid HUB_TYPE for build..."
#endif

class At88sc {
public:
    At88sc()
    : At88sc( AT88SC_i2cdevice ) {}
    At88sc( const std::string& devPath );
    bool write( i2c::Byte command, const i2c::ByteData& data );
    bool write( i2c::Byte command, const i2c::Byte msb, const i2c::Byte lsb, const std::string& data );
    i2c::ByteData read( i2c::Byte command, const i2c::Byte msb, const i2c::Byte lsb, const i2c::Byte size );
    bool writePassword();
    void blowFuses();
    bool saveSerial( const std::string& serial );
    std::string getSerial();
    bool saveMac( const std::string& mac );
    std::string getMac();
    bool saveRegion( const std::string& region );
    std::string getRegion();

private:
    const i2c::ByteData defaulPassword = { 0x07, 0x00, 0x03, 0xDD, 0x42, 0x97 };
    std::unique_ptr< i2c::I2cControl > i2cControl;
};
