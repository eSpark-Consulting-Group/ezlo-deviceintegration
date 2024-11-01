#pragma once

#include <termios.h>

namespace TwoGig {

struct BaudRate {
    enum Enum {
        Baud_0,
        Baud_50,
        Baud_75,
        Baud_110,
        Baud_134,
        Baud_150,
        Baud_200,
        Baud_300,
        Baud_600,
        Baud_1200,
        Baud_1800,
        Baud_2400,
        Baud_4800,
        Baud_9600,
        Baud_19200,
        Baud_38400
    };
};

struct Parity {
    enum Enum { None, Odd, Even };
};

struct CharacterSize {
    enum Enum { CS_5, CS_6, CS_7, CS_8 };
};

struct StopBitsCount {
    enum Enum { One, Two };
};

struct ConfigurationSettings {
    BaudRate::Enum baudRate;
    Parity::Enum parity;
    CharacterSize::Enum characterSize;
    StopBitsCount::Enum stopBitsCount;
    bool hardwareFlowControlEnabled;
};

termios getCustomConfiguration( const termios& prototype, const ConfigurationSettings& settings );

}  // namespace TwoGig
