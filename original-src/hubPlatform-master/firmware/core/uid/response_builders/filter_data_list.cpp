#include "filter_data_list.h"

namespace HubPlatform {
namespace Uid {

class FilterDataListBase : public FilterDataListI {
public:
    FilterDataListBase() = default;

    bool initIncludeFields( const JsonObject& fields ) override;
    bool hasIncludeField( const std::string& name ) const override;
    bool emptyIncludeFields() const override;

    bool initIds( const JsonObject& fields ) override;
    bool hasId( const std::string& id ) const override;
    bool emptyIds() const override;

    bool idsNotInSet( std::set< std::string > const& with, std::vector< std::string >& result ) const override;

    using StringSet = std::set< std::string >;
    static bool initFilter( StringSet& dict, const JsonObject& array );

protected:
    StringSet m_fields;
    StringSet m_ids;
};

bool FilterDataListBase::emptyIds() const {
    return m_ids.empty();
}

bool FilterDataListBase::hasId( const std::string& id ) const {
    if( m_ids.empty() )
        return true;
    return m_ids.find( id ) != end( m_ids );
}

bool FilterDataListBase::initFilter( StringSet& dict, const JsonObject& array ) {
    if( !array.is_array() )
        return false;
    size_t added = 0;
    for( auto it = begin( array ); it != end( array ); it++ ) {
        if( it->is_string() ) {
            const auto& name = it->get< std::string >();

            auto inserted = dict.insert( name );
            if( inserted.second )
                added++;
        }
        else {
            throw std::runtime_error( std::string( "initFilter expecting string array, got " ) +
                                      std::to_string( (int) it->type() ) );
        }
    }
    return added > 0;
}

bool FilterDataListBase::initIds( const JsonObject& ids ) {
    return initFilter( m_ids, ids );
}

bool FilterDataListBase::hasIncludeField( const std::string& name ) const {
    if( m_fields.empty() )
        return true;
    return m_fields.find( name ) != end( m_fields );
}

bool FilterDataListBase::emptyIncludeFields() const {
    return m_fields.empty();
}

bool FilterDataListBase::initIncludeFields( const JsonObject& fields ) {
    return initFilter( m_fields, fields );
}

/** Returns all stored ids, not included in 'with' collection. */
bool FilterDataListBase::idsNotInSet( const std::set< std::string >& with, std::vector< std::string >& result ) const {
    for( const auto& savedId : m_ids ) {
        if( with.find( savedId ) == end( with ) )
            result.push_back( savedId );
    }
    return !result.empty();
}

DataListFilterUPtr makeDataListFilter() {
    DataListFilterUPtr ptr( static_cast< FilterDataListI* >( new FilterDataListBase() ) );
    return ptr;
}

SetOfStrings makeAllIdsMap( const std::vector< JsonObject >& devices ) {
    SetOfStrings allIds;
    for( const auto& device : devices ) {
        std::string deviceId = json_get_str( device, "_id" );
        if( deviceId.empty() ) {
            throw std::runtime_error( "Fields device._id should not be empty!" );
        }
        allIds.insert( deviceId );
    }
    return std::move( allIds );
}

JsonObject makeFieldsDict( const JsonObject& obj, FilterDataListI* filter ) {
    JsonObject props;

    for( auto field = obj.begin(); field != obj.end(); ++field ) {
        if( field.key() == "_id" )
            continue;
        if( filter == nullptr || filter->hasIncludeField( field.key() ) )
            props[ field.key() ] = field.value();
    }
    return std::move( props );
}

// void fillResultWithMissedIds( JsonObject& result, const FilterDataListI* filterPtr, const std::vector<JsonObject>&
// collection ) {
void fillResultMissedIds( JsonObject& result,
                          const FilterDataListI* filterPtr,
                          const std::vector< JsonObject >& collection,
                          const JsonObject& defaultValue ) {
    if( nullptr == filterPtr || filterPtr->emptyIds() )
        return;

    std::set< std::string > allIds = makeAllIdsMap( collection );
    std::vector< std::string > difference;
    difference.reserve( allIds.size() );
    if( filterPtr->idsNotInSet( allIds, difference ) ) {
        for( const auto& queryId : difference ) {
            result[ queryId ] = defaultValue;
        }
    }
}

}  // namespace Uid
}  // namespace HubPlatform