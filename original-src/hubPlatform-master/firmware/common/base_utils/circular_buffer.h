#include <iostream>
#include <vector>

#include "optional.h"

namespace common {

template < typename T >
class CircularBuffer {
public:
    explicit CircularBuffer( size_t size )
    : m_maxSize( size )
    , m_head( 0 )
    , m_tail( 0 )
    , m_full( false ) {
        m_buffer.resize( m_maxSize );
    }

    void push( const T& item ) {
        m_buffer[ m_head ] = item;
        if( m_full ) {
            m_tail = ( m_tail + 1 ) % m_maxSize;
        }
        m_head = ( m_head + 1 ) % m_maxSize;
        m_full = m_head == m_tail;
    }

    optional< T > pop() {
        if( empty() ) {
            return optional< T >{};  // Return empty optional if buffer is empty
        }
        auto item = m_buffer[ m_tail ];
        m_full = false;
        m_tail = ( m_tail + 1 ) % m_maxSize;
        return item;
    }

    bool empty() const {
        return ( !m_full && ( m_head == m_tail ) );
    }

    bool isFull() const {
        return m_full;
    }

    size_t capacity() const {
        return m_maxSize;
    }

    size_t size() const {
        size_t size = m_maxSize;
        if( !m_full ) {
            if( m_head >= m_tail ) {
                size = m_head - m_tail;
            }
            else {
                size = m_maxSize + m_head - m_tail;
            }
        }
        return size;
    }

private:
    const size_t m_maxSize;
    size_t m_head;
    size_t m_tail;
    bool m_full;

    std::vector< T > m_buffer;
};

}  // namespace common
