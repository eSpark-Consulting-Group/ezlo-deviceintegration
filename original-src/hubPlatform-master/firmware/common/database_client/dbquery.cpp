#include "database_client/dbquery.h"

namespace db {

Query::Query( const Json& filter ) {
    if( filter.count( FILTER_FIELD ) ) {
        filter_ = filter.at( FILTER_FIELD );
        if( filter.count( ORDERBY_FIELD ) )
            orderBy_ = filter.at( ORDERBY_FIELD );
    }
    else {
        filter_ = filter;
    }
}

Query& Query::sort( const Json& orderBy ) {
    orderBy_ = orderBy;
    return *this;
}

Query& Query::sort( const std::string& orderBy, const int direction ) {
    return sort( JSON( orderBy << direction ) );
}

Query& Query::sort( const char* orderBy ) {
    return sort( JSON( orderBy << 1 ) );
}

Json Query::getJson() const {
    if( hasSort() )
        return JSON( FILTER_FIELD << filter_ << ORDERBY_FIELD << orderBy_ );
    else
        return filter_;
}

const Json& Query::getFilter() const {
    return filter_;
}

const Json& Query::getSort() const {
    return orderBy_;
}

bool Query::hasSort() const {
    return ( !orderBy_.empty() );
}

}  // namespace db
