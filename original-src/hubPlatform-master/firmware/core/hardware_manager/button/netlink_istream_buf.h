#pragma once

#include <streambuf>
#include <exception>
#include <linux/netlink.h>

class BadStreamBuf : public std::exception {
public:
    const char* what() const noexcept override;
};

class NetlinkIStreamBuf final : public std::streambuf {
public:
    static const int size = 512;

public:
    NetlinkIStreamBuf();
    ~NetlinkIStreamBuf();

protected:
    virtual int underflow();

private:
    int m_fd;
    struct sockaddr_nl m_addr;
    char m_buf[ size ];

private:
    void init();
};