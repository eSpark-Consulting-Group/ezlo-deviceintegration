#pragma once

#include <string>

namespace common {

enum class GpioState : int { OFF = 0, ON, UNDEFINED = -1 };
enum class GpioEdge : int { RISING = 0, FALLING, BOTH, NONE = -1 };
enum class GpioDirection : int { OUT, IN };

constexpr char GpioPath[] = "/sys/class/gpio/";

class Gpio {
public:
    Gpio( int gpioNumber );
    GpioState getState() const;
    bool setState( GpioState state );
    bool setDirection( GpioDirection direction );
    bool setEdgeDetecting( GpioEdge edge );

private:
    bool setGpio( int gpioNumber );
    bool isGpioSet( const std::string& gpioNumber );
    std::string m_gpioNumber;
    GpioDirection m_direction;
};

}  // namespace common
