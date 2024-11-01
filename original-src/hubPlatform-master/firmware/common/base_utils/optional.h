#pragma once

#include <cassert>
#include <cstdlib>
#include <utility>

namespace common {

template < typename T >
class optional {
public:
    using element_type = T;

    optional()
    : m_has_value{ false }
    , m_empty{} {}

    optional( const T& t )
    : m_has_value{ true }
    , m_value{ t } {}

    optional( T&& t )
    : m_has_value {
        true
    }
// Doesn't work on GCC 4.8.3. Compiler tries to initialize each member of structure instead of calling move constructor.
#if __GNUC__ > 4
    , m_value{ std::move( t ) } {}
#else
    , m_value( std::move( t ) ) {}
#endif

    optional( const optional& other )
    : m_has_value{ other.m_has_value } {
        if( m_has_value ) {
            new( &m_value ) T( other.m_value );
        }
        else {
            m_empty = {};
        }
    }

    optional( optional&& other ) noexcept( std::is_nothrow_move_constructible< T >::value )
    : m_has_value{ other.m_has_value } {
        if( m_has_value ) {
            new( &m_value ) T( std::move( other.m_value ) );
        }
        else {
            m_empty = {};
        }
    }

    ~optional() {
        reset();
    }

    optional& operator=( const T& other ) {
        reset( other );
        return *this;
    }

    optional& operator=( T&& other ) {
        reset( std::move( other ) );
        return *this;
    }

    optional& operator=( const optional& other ) {
        if( other.m_has_value ) {
            reset( other.m_value );
        }
        else {
            reset();
        }
        return *this;
    }

    optional& operator=( optional&& other ) noexcept( std::is_nothrow_move_assignable< T >::value ) {
        if( other.m_has_value ) {
            reset( std::move( other.m_value ) );
        }
        else {
            reset();
        }
        return *this;
    }

    void reset() {
        if( m_has_value ) {
            m_value.~T();
        }
        m_has_value = false;
        m_empty = {};
    }

    template < typename OtherT >
    void reset( OtherT&& other ) {
        if( m_has_value ) {
            m_value = std::forward< OtherT >( other );
        }
        else {
            m_has_value = true;
            new( &m_value ) T( std::forward< OtherT >( other ) );
        }
    }

    explicit operator bool() const {
        return m_has_value;
    }

    bool has_value() const {
        return m_has_value;
    }

    const T& operator*() const& {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return m_value;
    }

    T& operator*() & {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return m_value;
    }

    T&& operator*() && {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return std::move( m_value );
    }

    const T* operator->() const& {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return &m_value;
    }

    T* operator->() & {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return &m_value;
    }

    T* operator->() && {
        assert( m_has_value );
        if( !m_has_value ) {
            std::abort();  // instead of UB
        }
        return &m_value;
    }

    const T* get() const {
        return m_has_value ? &m_value : nullptr;
    }

    T* get() {
        return m_has_value ? &m_value : nullptr;
    }

private:
    bool m_has_value;

    struct empty_byte {};
    union {
        empty_byte m_empty;
        T m_value;
    };
};

template < typename T1, typename T2 >
inline bool operator==( const optional< T1 >& lhs, const optional< T2 >& rhs ) {
    return lhs ? ( rhs && *rhs == *lhs ) : !rhs;
}

template < typename T1, typename T2 >
inline bool operator!=( const optional< T1 >& lhs, const optional< T2 >& rhs ) {
    return !( lhs == rhs );
}

}  // namespace common
