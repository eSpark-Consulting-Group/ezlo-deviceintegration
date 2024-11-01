#if( !defined __TINY_ASIO_H__ )
#define __TINY_ASIO_H__ 1

#if( !defined ASIO_STANDALONE )
#define ASIO_STANDALONE 1
#define ASIO_HAS_STD_SYSTEM_ERROR 1
//    #if (defined __NETWORK_TRAFFIC_LOGGING__)
//        #define ASIO_ENABLE_HANDLER_TRACKING 1
//    #endif /// (defined __NETWORK_TRAFFIC_LOGGING__)
#endif  /// #if (!defined ASIO_STANDALONE)

#include <asio.hpp>

#endif  /// (! defined __TINY_ASIO_H__)
