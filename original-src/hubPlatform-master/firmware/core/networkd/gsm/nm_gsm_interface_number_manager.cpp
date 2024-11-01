#include "networkd/gsm/nm_gsm_interface_number_manager.h"
#include "networkd/resources/nm_resources.h"

#include "common_utils/cpp14.h"

#include <numeric>

namespace HubPlatform {
namespace Network {
namespace Mobile {

InterfaceNumber::InterfaceNumber( InterfaceNumberManager& interfaceNumberManager, bool interfaceEmbedded )
: m_interfaceNumberManager( interfaceNumberManager )
, m_interfaceEmbedded( interfaceEmbedded ) {
    m_interfaceNumber = m_interfaceNumberManager.acquireInterfaceNumber( m_interfaceEmbedded );
}

InterfaceNumber::~InterfaceNumber() {
    m_interfaceNumberManager.releaseInterfaceNumber( m_interfaceNumber, m_interfaceEmbedded );
}

int InterfaceNumber::getInterfaceNumber() const {
    return m_interfaceNumber;
}

InterfaceNumberManager::InterfaceNumberManager() {
    std::vector< int > digits( 16 );

    // reserve space for an embedded modem/interface
    std::iota( digits.begin(), digits.end(), Resources::GSM::EmbeddedCellularModemNumber + 1 );
    m_interfaceNumbersQueue = decltype( m_interfaceNumbersQueue )( std::greater< int >(), digits );
}

std::unique_ptr< InterfaceNumber > InterfaceNumberManager::allocateInterfaceNumber( bool interfaceEmbedded ) {
    return std::unique_ptr< InterfaceNumber >( new InterfaceNumber( *this, interfaceEmbedded ) );
}

int InterfaceNumberManager::acquireInterfaceNumber( bool interfaceEmbedded ) {
    std::lock_guard< std::mutex > lock( m_interfaceNumbersQueueMutex );
    int number;

    if( interfaceEmbedded ) {
        number = Resources::GSM::EmbeddedCellularModemNumber;
    }
    else {
        number = m_interfaceNumbersQueue.top();
        m_interfaceNumbersQueue.pop();
    }

    return number;
}

void InterfaceNumberManager::releaseInterfaceNumber( int interfaceNumber, bool interfaceEmbedded ) {
    std::lock_guard< std::mutex > lock( m_interfaceNumbersQueueMutex );

    if( !interfaceEmbedded ) {
        m_interfaceNumbersQueue.push( interfaceNumber );
    }
}

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
