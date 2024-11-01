#pragma once

#include "base_utils/non_copyable.h"

#include <string>
#include <unistd.h>
#include <termios.h>

namespace HubPlatform {
namespace Zwave {

class Uart final : public common::NonCopyable {
public:
    Uart( const std::string& tty );
    ~Uart();

public:
    bool isInitialized() const;
    int getInputBufferBitsCount( int& val );
    int read( void* data, size_t size );
    int write( const void* data, size_t size );
    bool isIoOk( int status );

    bool init();
    void deinit();

private:
    bool reset();

    bool open();
    bool configure();
    void close();
    void closeAndClearFd();

private:
    std::string m_ttyPath;
    int m_ttyFd;
    termios m_uartConfigBackup;
    termios m_uartConfig;
    bool m_isConfigured;
    bool m_isOpened;
};

}  // namespace Zwave
}  // namespace HubPlatform
