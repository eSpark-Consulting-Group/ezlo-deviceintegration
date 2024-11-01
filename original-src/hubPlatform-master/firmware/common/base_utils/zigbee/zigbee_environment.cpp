#include "base_utils/zigbee/zigbee_environment.h"

#include <fstream>
#include <thread>

namespace HubPlatform {
namespace Environment {
namespace Zigbee {

#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )
static constexpr char bootValueFilename[] = "/sys/class/gpio/gpio102/value";
static constexpr char bootDirectionFilename[] = "/sys/class/gpio/gpio102/direction";

static constexpr char resetValueFilename[] = "/sys/class/gpio/gpio362/value";
static constexpr char resetDirectionFilename[] = "/sys/class/gpio/gpio362/direction";

#elif defined( __HUB_TYPE_BEEWAVE__ )
static constexpr char bootValueFilename[] = "/sys/class/gpio/gpio11/value";
static constexpr char bootDirectionFilename[] = "/sys/class/gpio/gpio11/direction";

static constexpr char resetValueFilename[] = "/sys/class/gpio/gpio16/value";
static constexpr char resetDirectionFilename[] = "/sys/class/gpio/gpio16/direction";
#else

static constexpr char bootValueFilename[] = "/dev/null";
static constexpr char bootDirectionFilename[] = "/dev/null";

static constexpr char resetValueFilename[] = "/dev/null";
static constexpr char resetDirectionFilename[] = "/dev/null";
#endif

void bootInit() {
    std::ofstream bootDirectionFile( bootDirectionFilename, std::ofstream::binary );
    bootDirectionFile << "out";
    bootDirectionFile.flush();
    std::ofstream bootValueFile( bootValueFilename, std::ofstream::binary );
    bootValueFile << "1";
    bootValueFile.flush();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
}

void bootAssert() {
    std::ofstream bootFile( bootValueFilename, std::ofstream::binary );
    bootFile << "0";
    bootFile.flush();
}

void bootDeAssert() {
    std::ofstream bootFile( bootValueFilename, std::ofstream::binary );
    bootFile << "1";
    bootFile.flush();
}

void resetInit() {
    std::ofstream resetDirectionFile( resetDirectionFilename, std::ofstream::binary );
    resetDirectionFile << "out";
    resetDirectionFile.flush();
    resetDeAssert();
}

void resetAssert() {
    std::ofstream resetFile( resetValueFilename, std::ofstream::binary );
    resetFile << "0";
    resetFile.flush();
}

void resetDeAssert() {
    std::ofstream resetFile( resetValueFilename, std::ofstream::binary );
    resetFile << "1";
    resetFile.flush();
}

void resetCycle() {
    resetAssert();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    resetDeAssert();
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
}

}  // namespace Zigbee
}  // namespace Environment
}  // namespace HubPlatform
