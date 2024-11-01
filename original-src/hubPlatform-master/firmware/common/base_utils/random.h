#pragma once

#include <random>

namespace Common {

class Random {
public:
    Random( int from, int to );

    int operator()();

private:
    std::random_device randomDevice;               // obtain a random number from hardware
    std::mt19937 seedGenerator;                    // seed the generator
    std::uniform_int_distribution<> distribution;  // define the range
};

}  // namespace Common
