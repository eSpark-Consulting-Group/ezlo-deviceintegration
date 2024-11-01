#pragma once

#include "hardware_manager/button/netlink_istream_buf.h"
#include "hardware_manager/button/button_event.h"
#include <istream>

class NetlinkIStream final : public std::istream {
public:
    NetlinkIStream();
    NetlinkIStream& operator>>( ButtonEvent& evt );

private:
    NetlinkIStreamBuf m_netlinkStream;

private:
    void processLine( const std::string& line, ButtonEvent& evt ) const;
};