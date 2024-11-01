#include "stdin_log_reader.h"
#include "logreader/log_reader/log_reader_interface.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace LogReader {
void StdInLogReader::run( const CallbackFunc callbackFunc ) {
    LogReaderInterface::run( callbackFunc );
    std::string line;
    while( getline( std::cin, line ) ) {
        callbackFunc( line );
    }
}

}  // namespace LogReader
}  // namespace HubPlatform
