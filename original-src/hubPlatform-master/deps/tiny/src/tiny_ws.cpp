#include <tiny/tiny_ws.h>
#include <tiny/tiny_url.h>
#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_utils.h>
#include <tiny/tiny_network.h>
#include <sstream>
#include <chrono>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>

#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#if( defined _WIN32 )
#include <io.h>
#include "gettimeofday.h"
#else
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#if( defined __HAVE_TINY_WS_BACKEND_NLWS__ )
extern "C" {
//		#define LWS_PLUGIN_STATIC
//		#include "../../libwebsockets/plugins/protocol_lws_mirror.c"
//		#include "../../libwebsockets/plugins/protocol_lws_status.c"
//		#include "../../libwebsockets/plugins/protocol_lws_meta.c"
};      /// extern "C"
#endif  /// (defined __HAVE_TINY_WS_BACKEND_NLWS__)

namespace tiny {
namespace ws {

#include "tiny_ws/utils.cpp"

#include "tiny_ws/traffic_stats.cpp"
#include "tiny_ws/basic_network_settings.cpp"
#include "tiny_ws/generic_participant.cpp"

#if( defined __HAVE_TINY_WS_BACKEND_NLWS__ )
namespace nlws {

#include "tiny_ws/message_payload_data.cpp"
#include "tiny_ws/basic_api.cpp"
#include "tiny_ws/client_api.cpp"
#include "tiny_ws/server_api.cpp"
#include "tiny_ws/basic_participant.cpp"
#include "tiny_ws/basic_sender.cpp"
#include "tiny_ws/basic_socket.cpp"
#include "tiny_ws/peer.cpp"
#include "tiny_ws/server.cpp"
#include "tiny_ws/client.cpp"

};      // namespace nlws
#endif  /// (defined __HAVE_TINY_WS_BACKEND_NLWS__)

}  // namespace ws
}  // namespace tiny