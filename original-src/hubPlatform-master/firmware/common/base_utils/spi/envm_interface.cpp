#include "base_utils/spi/envm_interface.h"

namespace envm {

std::string hexToString( const unsigned char* data, int size, bool upperCase ) {
    std::ostringstream result;
    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( int i = 0; i < size; ++i ) {
        result << std::setw( 2 ) << (int) data[ i ];
    }
    return result.str();
}

}  // namespace envm

static EnvmMockTransport mockTransport{};
