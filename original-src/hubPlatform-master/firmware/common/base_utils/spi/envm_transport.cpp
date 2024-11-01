#include "base_utils/spi/envm_transport.h"

#include <fstream>

int32_t EnvmMockTransport::init( uint32_t spiSpeed, const bool keepCSActivated ) {
    return 0;
}

void EnvmMockTransport::deinit() {}

int EnvmMockTransport::sendRequest( const std::vector< uint8_t >& command,
                                    int32_t resultSize,
                                    std::vector< uint8_t >& outResult ) {
    return 0;
}

bool EnvmMockTransport::isValid() const {
    return true;
}
