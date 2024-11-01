#include "base_utils/spi/sercomm.h"

#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

namespace Sercomm {
constexpr char const GpioIoctlDevice[] = "/dev/sc_gpio_ioctl";

constexpr int returnValueOk = 0;
constexpr int returnValueError = -1;

// ioctl command codes from sercomm gpio driver
//
enum {
    CMD_TEST_NUM = 0x02,
    CMD_GPIO_IN_SET,
    CMD_GPIO_OUT_SET,
    CMD_GPIO_DATA_SET,
    CMD_GPIO_DATA_CLEAR,
    CMD_GPIO_DATA_GET,
    CMD_GPIO_RESET_HIGH,
    CMD_GPIO_RESET_LOW,
};

namespace {

bool validatePin( int pinNumber ) {
    const bool result = ( pinNumber > MinPinNumber && pinNumber <= MaxPinNumber );
    if( !result ) {
        std::cerr << "Failed to validate pin: " << pinNumber << " - out of valid range." << std::endl;
    }
    return result;
}

}  // namespace

int openGpioDevice( int fdGpioDevice ) {
    if( fdGpioDevice < 0 ) {
        fdGpioDevice = open( GpioIoctlDevice, O_RDWR );
        if( fdGpioDevice < 0 ) {
            char* error = strerror( errno );
            std::cerr << "GPIO device open failed: errno = " << (int) errno << " - " << error << std::endl;
            return returnValueError;
        }
    }

    return fdGpioDevice;
}

int gpioDirectionIn( int gpioNumber, int fdGpioDevice ) {
    int cmd = CMD_GPIO_IN_SET;

    if( !validatePin( gpioNumber ) ) {
        return returnValueError;
    }

    fdGpioDevice = openGpioDevice( fdGpioDevice );
    if( fdGpioDevice < 0 ) {
        return returnValueError;
    }

    if( ioctl( fdGpioDevice, cmd, &gpioNumber ) < 0 ) {
        std::cerr << "Can't set gpio as input: pin " << gpioNumber << std::endl;
        return returnValueError;
    }
    return returnValueOk;
}

int gpioDirectionOut( int gpioNumber, int fdGpioDevice ) {
    const int cmd = CMD_GPIO_OUT_SET;

    if( !validatePin( gpioNumber ) ) {
        return returnValueError;
    }

    fdGpioDevice = openGpioDevice( fdGpioDevice );
    if( fdGpioDevice < 0 ) {
        return returnValueError;
    }

    if( ioctl( fdGpioDevice, cmd, &gpioNumber ) < 0 ) {
        std::cerr << "Can't set gpio as output: pin " << gpioNumber << std::endl;
        return returnValueError;
    }

    return returnValueOk;
}

int gpioReadValue( int gpioNumber, int fdGpioDevice ) {
    if( !validatePin( gpioNumber ) ) {
        return returnValueError;
    }

    fdGpioDevice = openGpioDevice( fdGpioDevice );
    if( fdGpioDevice < 0 ) {
        return returnValueError;
    }

    const int cmd = CMD_GPIO_DATA_GET;
    int value = gpioNumber;
    if( ioctl( fdGpioDevice, cmd, &value ) < 0 ) {
        std::cerr << "Can't read gpio value: pin " << gpioNumber << std::endl;
        return returnValueError;
    }
    return value;
}

int gpioWriteValue( int gpioNumber, unsigned int gpioValue, int fdGpioDevice ) {
    if( !validatePin( gpioNumber ) ) {
        return returnValueError;
    }

    fdGpioDevice = openGpioDevice( fdGpioDevice );
    if( fdGpioDevice < 0 ) {
        return returnValueError;
    }

    const int cmd = ( gpioValue > 0 ) ? CMD_GPIO_DATA_SET : CMD_GPIO_DATA_CLEAR;
    if( ioctl( fdGpioDevice, cmd, &gpioNumber ) < 0 ) {
        std::cerr << "Can't write gpio value: pin " << gpioNumber << std::endl;
        return returnValueError;
    }
    return returnValueOk;
}

}  // namespace Sercomm