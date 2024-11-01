#pragma once

namespace Sercomm {

constexpr int ZwaveRstPinNumber = 61;

constexpr int MisoPinNumber = 62;
constexpr int SckPinNumber = 63;
constexpr int MosiPinNumber = 64;
constexpr int CsPinNumber = 65;

constexpr int MinPinNumber = 0;
constexpr int MaxPinNumber = 65;

int openGpioDevice( int fdGpioDevice = -1 );
int gpioDirectionIn( int gpioNumber, int fdGpioDevice = -1 );
int gpioDirectionOut( int gpioNumber, int fdGpioDevice = -1 );
int gpioReadValue( int gpioNumber, int fdGpioDevice = -1 );
int gpioWriteValue( int gpioNumber, unsigned int gpioValue, int fdGpioDevice = -1 );

}  // namespace Sercomm