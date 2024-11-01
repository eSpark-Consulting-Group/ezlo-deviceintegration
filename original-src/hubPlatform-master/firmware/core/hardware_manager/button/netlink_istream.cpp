#include "hardware_manager/button/netlink_istream.h"

namespace {
const char ButtonSearchKey[] = "BUTTON=";
const char SecondsPassedSearchKey[] = "SEEN=";
const char PressedSearchKey[] = "pressed@";
const char ReleasedSearchKey[] = "released@";
}  // namespace

void NetlinkIStream::processLine( const std::string& line, ButtonEvent& evt ) const {
    auto pos = std::string::npos;
    if( line == PressedSearchKey ) {
        evt.setType( ButtonEventType::Pressed );
    }
    else if( line == ReleasedSearchKey ) {
        evt.setType( ButtonEventType::Released );
    }
    else if( ( pos = line.find( ButtonSearchKey ) ) != std::string::npos ) {
        evt.setName( line.substr( pos + sizeof( ButtonSearchKey ) - 1 ) );
    }
    else if( ( pos = line.find( SecondsPassedSearchKey ) ) != std::string::npos ) {
        evt.setSecondsPassed( std::stoi( line.substr( pos + sizeof( SecondsPassedSearchKey ) - 1 ) ) );
    }
}

NetlinkIStream& NetlinkIStream::operator>>( ButtonEvent& evt ) {
    std::string currentLine;
    while( std::getline( *this, currentLine, '\0' ) ) {
        processLine( currentLine, evt );
    }
    clear();
    if( evt.getType() == ButtonEventType::Unknown ) {
        *this >> evt;
    }
    return *this;
}

NetlinkIStream::NetlinkIStream()
: std::istream( 0 )
, m_netlinkStream() {
    rdbuf( &m_netlinkStream );
}