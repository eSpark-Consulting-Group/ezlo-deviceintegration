#include "hardware_manager/button/vera/vera_buttons_handler.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

namespace HubPlatform {
namespace HardwareManager {

namespace {
constexpr char ButtonSearchKey[] = "BUTTON=";
constexpr char SecondsPassedSearchKey[] = "SEEN=";
constexpr char PressedSearchKey[] = "pressed@";
constexpr char ReleasedSearchKey[] = "released@";
}  // namespace

VeraButtonsHandler& VeraButtonsHandler::instance() {
    static VeraButtonsHandler veraButtonsHandler;
    return veraButtonsHandler;
}

VeraButtonsHandler::~VeraButtonsHandler() {
    if( m_socketDesc != -1 )
        close( m_socketDesc );
}

void VeraButtonsHandler::listen() {
    sockaddr_nl saddr = {};
    m_socketDesc = socket( AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT );
    if( m_socketDesc == -1 ) {
        throw Exception( std::string( "Failed to create socket AF_NETLINK: " ) + strerror( errno ) );
    }

    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = getpid();
    saddr.nl_groups = -1;

    if( bind( m_socketDesc, reinterpret_cast< sockaddr* >( &saddr ), sizeof( saddr ) ) == -1 ) {
        throw Exception( std::string( "Failed to bind socket AF_NETLINK: " ) + strerror( errno ) );
    }
}

void VeraButtonsHandler::processButtons() {
    std::string buf( 512, '\0' );
    const ssize_t result = recv( m_socketDesc, &buf[ 0 ], buf.length(), MSG_DONTWAIT );
    if( result == -1 ) {
        if( errno == EINTR || errno == EAGAIN ) {
            return;
        }
        throw Exception( std::string( "Failed to read from socket AF_NETLINK: " ) + strerror( errno ) );
    }
    buf.resize( result );
    parseNetlinkEvent( buf );
}

void VeraButtonsHandler::addButtonStateHandler( const std::string& buttonName,
                                                VeraButtonsHandler::OnButtonStateChanged buttonStateHandler ) {
    m_buttonHandlers.emplace( buttonName, std::move( buttonStateHandler ) );
}

void VeraButtonsHandler::removeButtonStateHandler( const std::string& buttonName ) {
    m_buttonHandlers.erase( buttonName );
}

void VeraButtonsHandler::parseNetlinkEvent( const std::string& data ) {
    if( data.size() < 1 )
        return;

    std::stringstream ss( data );
    std::string line, buttonName;
    ButtonState::StateValue buttonState = ButtonState::StateValue::Unknown;
    uint secondsPressed = 0;

    while( std::getline( ss, line, '\0' ) ) {
        auto pos = std::string::npos;
        if( line == PressedSearchKey ) {
            buttonState = ButtonState::StateValue::Pressed;
        }
        else if( line == ReleasedSearchKey ) {
            buttonState = ButtonState::StateValue::Released;
        }
        else if( ( pos = line.find( ButtonSearchKey ) ) != std::string::npos ) {
            buttonName = line.substr( pos + sizeof( ButtonSearchKey ) - 1 );
        }
        else if( ( pos = line.find( SecondsPassedSearchKey ) ) != std::string::npos ) {
            secondsPressed = std::stoul( line.substr( pos + sizeof( SecondsPassedSearchKey ) - 1 ) );
        }
    }

    auto it = m_buttonHandlers.find( buttonName );
    if( it != m_buttonHandlers.end() ) {
        it->second( buttonState, secondsPressed );
    }
}

}  // namespace HardwareManager
}  // namespace HubPlatform
