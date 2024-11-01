#pragma once

#include <tuple>

namespace Details {

template < typename Tuple, size_t Index = std::tuple_size< typename std::decay< Tuple >::type >::value >
struct ElementsHandler {
    template < typename F >
    static void forEach( Tuple&& t, F&& f ) {
        f( std::get< std::tuple_size< typename std::decay< Tuple >::type >::value - Index >( t ) );
        ElementsHandler< Tuple, Index - 1 >::forEach( std::forward< Tuple >( t ), f );
    }
};

template < typename Tuple >
struct ElementsHandler< Tuple, 0 > {
    template < typename... Args >
    static void forEach( Args&&... ) {}
};

}  // namespace Details

template < typename Tuple, typename F >
inline void foreachInTuple( Tuple&& t, F&& f ) {
    Details::ElementsHandler< Tuple >::forEach( std::forward< Tuple >( t ), std::forward< F >( f ) );
}
