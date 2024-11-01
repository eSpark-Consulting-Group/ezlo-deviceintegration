#pragma once

#include "json_utils/json_wrapper.h"

namespace db {

static const std::string FILTER_FIELD = "$filter";
static const std::string ORDERBY_FIELD = "$orderby";

class Query {
public:
    Query()
    : filter_( JSON_EMPTY_OBJ ) {}
    Query( const Json& filter );
    Query& sort( const Json& orderBy );
    Query& sort( const std::string& orderBy, const int direction = 1 );
    Query& sort( const char* orderBy );
    Json getJson() const;
    const Json& getFilter() const;
    const Json& getSort() const;
    bool hasSort() const;

private:
    Json filter_;
    Json orderBy_;
};

}  // namespace db
