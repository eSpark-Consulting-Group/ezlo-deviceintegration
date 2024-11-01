#pragma once

#include <string>
#include <set>
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Uid {

class FilterDataListI {
public:
    virtual ~FilterDataListI() = default;

    virtual bool initIncludeFields( const JsonObject& fields ) = 0;
    virtual bool hasIncludeField( const std::string& name ) const = 0;
    virtual bool emptyIncludeFields() const = 0;

    virtual bool initIds( const JsonObject& ids ) = 0;
    virtual bool hasId( const std::string& id ) const = 0;
    virtual bool emptyIds() const = 0;
    virtual bool idsNotInSet( const std::set< std::string >& with, std::vector< std::string >& result ) const = 0;
};

using DataListFilterUPtr = std::unique_ptr< FilterDataListI >;

DataListFilterUPtr makeDataListFilter();

using SetOfStrings = std::set< std::string >;

SetOfStrings makeAllIdsMap( const std::vector< JsonObject >& devices );
JsonObject makeFieldsDict( const JsonObject& obj, FilterDataListI* filter );
void fillResultMissedIds( JsonObject& result,
                          const FilterDataListI* filterPtr,
                          const std::vector< JsonObject >& collection,
                          const JsonObject& defaultValue );

}  // namespace Uid
}  // namespace HubPlatform