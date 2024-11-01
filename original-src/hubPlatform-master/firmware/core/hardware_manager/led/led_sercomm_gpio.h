#pragma once

#include "hardware_manager/led/led.h"
#include <string>

class LedSercommGPIO final : public Led {
public:
    static std::string procfsHandler; /*this needs to be taken from config*/
public:
    LedSercommGPIO( std::string name, int id );
    bool turnOn() override;
    bool turnOff() override;
    bool turnBlinking( int msOff, int msOn ) override;

private:
    int m_id;
};