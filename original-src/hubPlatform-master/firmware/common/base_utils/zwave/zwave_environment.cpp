#include "base_utils/zwave/zwave_environment.h"
#include "base_utils/spi/sercomm.h"
#include "base_utils/spi/actual_transport.h"

#include <unistd.h>
#include <fstream>
#include <unordered_map>

namespace HubPlatform {
namespace Environment {
namespace Zwave {

static constexpr char const RegionNameANZ[] = "ANZ";
static constexpr char const RegionNameCN[] = "CN";
static constexpr char const RegionNameEU[] = "EU";
static constexpr char const RegionNameHK[] = "HK";
static constexpr char const RegionNameIL[] = "IL";
static constexpr char const RegionNameIN[] = "IN";
static constexpr char const RegionNameJP[] = "JP";
static constexpr char const RegionNameKR[] = "KR";
static constexpr char const RegionNameRU[] = "RU";
static constexpr char const RegionNameUS[] = "US";

//  TODO: Inspect howto determine other regions correctly if frequencies are similar or nearby.
uint32_t const frequencyDeviationKHz = 30;
const std::unordered_map< uint32_t, const std::string > frequencyToRegionCodeMap = {
    // Frequency - region code( string ) :channel speed
    { 921420, RegionNameANZ },  // 9,6 kbps
    { 921400, RegionNameANZ },  // 40  kbps
    { 919800, RegionNameANZ },  // 100 kbps
    { 868420, RegionNameEU },   // 9,6 kbps
    { 868400, RegionNameEU },   // 40  kbps
    { 869850, RegionNameEU },   // 100 kbps
    { 908420, RegionNameUS },   // 9,6 kbps
    { 908400, RegionNameUS },   // 40  kbps
    { 916000, RegionNameUS },   // 100 kbps
};

const std::unordered_map< std::string, uint32_t > regionToFrequencyMap = {
    // region code( string ) - Frequency
    { RegionNameANZ, 921420 },
    { RegionNameEU, 868420 },
    { RegionNameUS, 908420 },
};

void resetAssert() {
#if( defined( __HUB_TYPE_G150__ ) )
    auto fdGpioDevice = Sercomm::openGpioDevice();
    Sercomm::gpioDirectionOut( Sercomm::ZwaveRstPinNumber );
    Sercomm::gpioWriteValue( Sercomm::ZwaveRstPinNumber, 0, fdGpioDevice );
    close( fdGpioDevice );
#else
    std::ofstream resetFile( resetDeviceFilename, std::ofstream::binary );
    resetFile << resetAssertCmd;
    resetFile.flush();
#endif
}

void resetDeAssert() {
#if( defined( __HUB_TYPE_G150__ ) )
    auto fdGpioDevice = Sercomm::openGpioDevice();
    Sercomm::gpioDirectionOut( Sercomm::ZwaveRstPinNumber );
    Sercomm::gpioWriteValue( Sercomm::ZwaveRstPinNumber, 1, fdGpioDevice );
    close( fdGpioDevice );
#else
    std::ofstream resetFile( resetDeviceFilename, std::ofstream::binary );
    resetFile << resetDeassertCmd;
    resetFile.flush();
#endif
}

void resetCycle( size_t usDelayAfter ) {
    resetDeAssert();
    usleep( 100000 );
    resetAssert();
    usleep( 100000 );
    resetDeAssert();
    usleep( usDelayAfter );
}

}  // namespace Zwave
}  // namespace Environment
}  // namespace HubPlatform
