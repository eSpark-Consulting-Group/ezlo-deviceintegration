#include <tiny/tiny_mail.h>
#include <tiny/tiny_utils.h>
#include <openssl/err.h>
#include <cassert>

#define TIME_IN_SEC 3 * 60  // how long client will wait for server response in non-blocking mode
#define BUFFER_SIZE 10240   // sendData and RecvData buffers sizes
#define MSG_SIZE_IN_MB 25   // the maximum size of the message with all attachments
#define COUNTER_VALUE 100   // how many times program will try to receive data

#if( defined WIN32 )
#else  /// (defined WIN32)
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#ifndef HAVE__STRNICMP
#define HAVE__STRNICMP
#define _strnicmp strncasecmp
#endif
#define OutputDebugStringA( buf )
#endif  /// else from (defined WIN32)
