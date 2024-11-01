#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "log_utils/ezlo_log.h"

#define PORT 3263
#define MAXLINE 1024

class FakeUdpServer final {
public:
    ~FakeUdpServer() {
        stopServer();
    }
    bool initUdpServer() {
        // Creating socket file descriptor
        if( ( m_sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
            return false;
        }

        fcntl( m_sockfd, F_SETFL, O_NONBLOCK );  // Set to non-blocking mode

        memset( &m_serverAddr, 0, sizeof( m_serverAddr ) );

        // Filling server information
        m_serverAddr.sin_family = AF_INET;  // IPv4
        m_serverAddr.sin_addr.s_addr = INADDR_ANY;
        m_serverAddr.sin_port = htons( PORT );

        // Bind the socket with the server address
        if( bind( m_sockfd, (const struct sockaddr*) &m_serverAddr, sizeof( m_serverAddr ) ) < 0 ) {
            return false;
        }

        return true;
    }

    bool readServer( int& length ) {
        socklen_t socketLen;
        m_receivedDataLen =
            recvfrom( m_sockfd, (char*) buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*) &m_serverAddr, &socketLen );
        if( m_receivedDataLen == -1 ) {
            return false;
        }

        length = socketLen;
        buffer[ m_receivedDataLen ] = '\0';

        return true;
    }

    void stopServer() {
        if( m_sockfd > -1 ) {
            close( m_sockfd );
            m_sockfd = -1;
        }
    }

private:
    int m_sockfd{ -1 };
    int m_receivedDataLen;
    struct sockaddr_in m_serverAddr;
    char buffer[ MAXLINE ];
};
