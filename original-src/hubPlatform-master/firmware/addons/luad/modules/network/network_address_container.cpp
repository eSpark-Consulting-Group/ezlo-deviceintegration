#include "luad/modules/network/network_address_container.h"

namespace HubPlatform {
namespace LuaNetwork {

AddressRange::AddressRange( AddressRange::AddressPtr&& min, AddressRange::AddressPtr&& max )
: m_min( std::move( min ) )
, m_max( std::move( max ) )
, m_isFinished( false ) {
    m_first = m_min->getAddress();
}

bool AddressRange::next() {
    if( *m_min < *m_max ) {
        ++( *m_min );
        return true;
    }
    m_isFinished = true;
    return false;
}

bool AddressRange::isFinished() {
    return m_isFinished;
}

IAddressContainer::AddressPtr AddressRange::getCurrent() {
    return m_min;
}

IAddressContainer::AddressPtr AddressRange::getLast() {
    return m_max;
}

void AddressRange::setFirst() {
    m_isFinished = false;
    m_min->setAddress( m_first );
}

AddressList::AddressList( AddressList::Addresses&& addresses )
: m_addresses( std::move( addresses ) )
, m_iterator( 0 )
, m_isFinished( false ) {}

bool AddressList::next() {
    if( ++m_iterator < m_addresses.size() ) {
        return true;
    }
    m_isFinished = true;
    --m_iterator;
    return false;
}

bool AddressList::isFinished() {
    return m_isFinished;
}

IAddressContainer::AddressPtr AddressList::getCurrent() {
    if( m_addresses.empty() ) {
        return nullptr;
    }
    return m_addresses[ m_iterator ];
}

IAddressContainer::AddressPtr AddressList::getLast() {
    return m_addresses.back();
}

void AddressList::setFirst() {
    m_isFinished = false;
    m_iterator = 0;
}

bool operator<( const IAddress& address1, const IAddress& address2 ) {
    return address1.m_address < address2.m_address;
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
