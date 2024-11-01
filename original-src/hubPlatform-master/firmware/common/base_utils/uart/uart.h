#pragma once

#include "base_utils/non_copyable.h"

#include <string>
#include <unistd.h>
#include <termios.h>

class Uart final : public common::NonCopyable {
public:
    Uart( const std::string& tty,
          const bool nonBlocking = false,
          bool hwFlowControl = false,
          bool throwException = false );
    ~Uart();

public:
    bool isInitialized() const;
    int getInputBufferBitsCount( int& val );
    int read( void* data, size_t size );
    int write( const void* data, size_t size );
    bool isIoOk( int status );
    int flush();

private:
    bool open( const bool nonBloking = false );
    bool configure( bool hwFlowControl );
    void close();
    void closeAndClearFd();

private:
    std::string m_ttyPath;
    int m_ttyFd;
    termios m_uartConfigBackup;
    bool m_isConfigured;
    bool m_isOpened;
    bool m_isNonBlocking;
};
