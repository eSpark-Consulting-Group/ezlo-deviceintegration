
#include "bus/spread_bus.h"
#include "version/ha_version.h"

#include <mutex>

#include "system_utils/environment.h"
#include "system_utils/logger.h"

constexpr std::chrono::seconds reconnectInterval{ 15 };

SpreadBus::SpreadBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts )
: BaseBus( pname, rxCallback )
, m_spreadName( HubPlatform::Environment::getBusName() )
, m_stopping( false ) {
    using namespace HubPlatform::SystemUtils;

    if( !silent ) {
        logInfo( &std::cout ) << getPrivateName() << ": " << HubPlatform::Version::ha_version() << std::endl;
    }

    m_timeout.sec = 5;
    m_timeout.usec = 0;

    char privateGroup[ MAX_GROUP_NAME ] = { 0 };
    int busConnectedState = COULD_NOT_CONNECT;

    for( uint8_t attempts = 0; attempts < connectionAttempts; ++attempts ) {
        if( attempts != 0 ) {
            std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
        }
        busConnectedState = SP_connect_timeout(
            m_spreadName.c_str(), getPrivateName().c_str(), 0, 1, &m_mbox, privateGroup, m_timeout );
        if( busConnectedState == ACCEPT_SESSION ) {
            break;
        }
        else {
            logWarn( &std::cout ) << getPrivateName() << ": " << std::to_string( attempts + 1 ) << "/"
                                  << std::to_string( connectionAttempts ) << ": ";
            SP_error( busConnectedState );
            std::cout << std::flush;
        }
    }

    if( busConnectedState != ACCEPT_SESSION ) {
        throw std::runtime_error( "Can not connect to spread" );
    }

    if( !silent ) {
        logInfo( &std::cout ) << getPrivateName() << ": Spread: connected to \"" << m_spreadName
                              << "\" with private group \"" << privateGroup << "\"" << std::endl;
    }

    join( getPrivateName().c_str() );
    join( "ha_bus" );
}

void SpreadBus::start() {
    m_thread = std::thread( &SpreadBus::run, this );
}

void SpreadBus::join( const char* group ) {
    const auto result = SP_join( m_mbox, group );
    if( result != 0 ) {
        HubPlatform::SystemUtils::logError() << getPrivateName() << ": ";
        SP_error( result );
    }
}

void SpreadBus::stop() {
    m_stopping = true;
    SP_disconnect( m_mbox );
    if( m_thread.joinable() ) {
        m_thread.join();
    }
}

void SpreadBus::run() {
    while( !m_stopping ) {
        rx_data.msg_size = SP_receive( m_mbox,
                                       &rx_data.service_type,
                                       rx_data.sender,
                                       Limits::MaxGroups,
                                       &rx_data.num_groups,
                                       rx_data.target_groups,
                                       &rx_data.msg_type,
                                       &rx_data.endian_mismatch,
                                       sizeof( rx_data.msg ),
                                       rx_data.msg );
        if( rx_data.msg_size < 0 ) {
            if( !m_stopping ) {
                HubPlatform::SystemUtils::logError() << getPrivateName() << ": ";
                SP_error( rx_data.msg_size );

                if( rx_data.msg_size == CONNECTION_CLOSED ) {
                    reconnect();
                    continue;
                }
            }
            break;
        }

        const auto& callback = getRxCallback();
        if( callback )
            callback( &rx_data );
    }
}

bool SpreadBus::multicast( const char* group, int msgClass, const std::string& msg ) {
    checkMessageLength( msg.size() );
    const int res = SP_multicast( m_mbox, FIFO_MESS, group, msgClass, msg.size(), msg.c_str() );
    checkMulticastError( msg, res );
    return ( res != static_cast< int >( msg.size() ) );
}

bool SpreadBus::isRegularMessage( int msgType ) const {
    return Is_regular_mess( msgType );
}

void SpreadBus::checkMulticastError( const std::string& message, int res ) const {
    if( res < 0 ) {
        using namespace HubPlatform::SystemUtils;
        using namespace std;

        if( ILLEGAL_SESSION == res )
            logError() << "multicast error: Illegal session" << endl;
        else if( ILLEGAL_MESSAGE == res )
            logError() << "multicast error: Illegal message" << endl;
        else if( CONNECTION_CLOSED == res )
            logError() << "multicast error: Connection closed" << endl;
        else if( MESSAGE_TOO_LONG == res )
            logError() << "multicast error: Message too long: " << message.size() << endl;
        else {
            logError() << "multicast error: some error: (" << res << "): ";
            SP_error( res );
            std::cerr << endl;
        }
    }
}

int SpreadBus::maxMessageLength() const {
    return static_cast< int >( Limits::MaxMulticastMessageLength );
}

void SpreadBus::reconnect() {
    using namespace HubPlatform::SystemUtils;

    logInfo( &std::cout ) << getPrivateName() << ": Spread trying to reconnect to \"" << m_spreadName << "\"\n"
                          << std::flush;

    SP_disconnect( m_mbox );

    while( !m_stopping ) {
        char privateGroup[ MAX_GROUP_NAME ] = { 0 };
        int busConnectedState = COULD_NOT_CONNECT;

        busConnectedState = SP_connect_timeout(
            m_spreadName.c_str(), getPrivateName().c_str(), 0, 1, &m_mbox, privateGroup, m_timeout );
        if( busConnectedState != ACCEPT_SESSION ) {
            logWarn( &std::cout ) << getPrivateName()
                                  << ": Spread client is unable to reconnect to spread bus: " << m_spreadName << ": ";
            SP_error( busConnectedState );
            std::cout << std::flush;

            const std::chrono::seconds timeTick{ 1 };
            for( std::chrono::seconds time{ 0 }; time < reconnectInterval && !m_stopping; time += timeTick )
                std::this_thread::sleep_for( timeTick );
            continue;
        }

        join( getPrivateName().c_str() );
        join( "ha_bus" );

        logInfo( &std::cout ) << getPrivateName() << ": Spread: reconnected to \"" << m_spreadName
                              << "\" with private group \"" << privateGroup << "\"" << std::endl;
        break;
    }
}
