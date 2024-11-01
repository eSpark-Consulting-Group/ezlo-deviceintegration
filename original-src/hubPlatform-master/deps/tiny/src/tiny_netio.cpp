#include <tiny/tiny_netio.h>
#include <tiny/tiny_utils.h>

extern "C" {
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>

#include <uv.h>
};  /// extern "C"
