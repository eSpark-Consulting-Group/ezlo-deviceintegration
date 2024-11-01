#pragma once

#include <array>
#include <algorithm>
#include <bitset>
#include <exception>
#include <type_traits>

/* @brief Light-weight wrapper for mapping sequential enum values in range [ 0; Enum::count )
 *        to some objects. EnumMapper doesn't use heap so the memory consumption is the same as for simple array.
 *        Supports reverse mapping of object members to enum.
 *        Expected usage: static mappings.
 */
template < typename Enum, typename Type >
class EnumMapper {
public:
    static_assert( std::is_enum< Enum >::value, "Please, pass enum class to template param" );

    constexpr static const auto Size = static_cast< size_t >( Enum::Count );
    using const_iterator = typename std::array< Type, Size >::const_iterator;

    template < typename MemberGetter,
               typename IsLess = std::less< decltype( std::declval< MemberGetter >()( std::declval< Type >() ) ) > >
    class ReverseMapper {
    public:
        using Member = decltype( std::declval< MemberGetter >()( std::declval< Type >() ) );

        ReverseMapper( const EnumMapper& mapper, MemberGetter&& memberGetter, IsLess&& isLess )
        : m_mapper( mapper )
        , m_memberGetter( std::move( memberGetter ) )
        , m_isLess( std::move( isLess ) ) {
            for( size_t index = 0; index < Size; ++index ) {
                m_indexes[ index ] = index;
            }

            std::sort( m_indexes.begin(), m_indexes.end(), [this]( size_t a, size_t b ) {
                return m_isLess( m_memberGetter( m_mapper.m_storage[ a ] ), m_memberGetter( m_mapper.m_storage[ b ] ) );
            } );
        }

        Enum find( const Member& member ) const {
            const auto found = std::lower_bound(
                m_indexes.begin(), m_indexes.end(), member, [this]( size_t index, const Member& member ) {
                    return m_isLess( m_memberGetter( m_mapper.m_storage[ index ] ), member );
                } );

            if( found != m_indexes.end() && !m_isLess( member, m_memberGetter( m_mapper.m_storage[ *found ] ) ) ) {
                return static_cast< Enum >( *found );
            }

            return Enum::Count;
        }

    private:
        const EnumMapper& m_mapper;
        std::array< size_t, Size > m_indexes{};
        MemberGetter m_memberGetter;
        IsLess m_isLess;
    };

    EnumMapper( std::initializer_list< std::pair< Enum, Type > > initializer ) {
        if( initializer.size() != Size ) {
            throw std::logic_error( "Wrong initializer size" );
        }

        std::bitset< Size > initializationFlags;

        for( auto& pair : initializer ) {
            const auto index = static_cast< size_t >( pair.first );
            if( index >= Size ) {
                throw std::logic_error( "Wrong enum value" );
            }

            if( initializationFlags[ index ] ) {
                throw std::logic_error( "Duplicated enum value" );
            }

            initializationFlags[ index ] = true;
            m_storage[ index ] = std::move( pair.second );
        }
    }

    template < typename MemberGetter, typename IsLess >
    ReverseMapper< MemberGetter, IsLess > createReverseMapper( MemberGetter memberGetter, IsLess isLess ) const {
        return ReverseMapper< MemberGetter, IsLess >( *this, std::move( memberGetter ), std::move( isLess ) );
    }

    const_iterator find( Enum enumIndex ) const {
        return m_storage.begin() + std::min( static_cast< size_t >( enumIndex ), m_storage.size() );
    }

    const_iterator begin() const {
        return m_storage.begin();
    }

    const_iterator end() const {
        return m_storage.end();
    }

private:
    std::array< Type, Size > m_storage{};
};
