#include "json_utils/json_fwd.h"
#include "filter_data_list.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {

class SetFieldHelper {
public:
    SetFieldHelper( JsonObject& dest, FilterDataListI* filter = nullptr )
    : m_dest( dest )
    , m_filter( filter ) {}

    template < typename ValueType >
    bool set( const std::string& field, const ValueType& value ) {
        if( filter( field ) ) {
            m_dest[ field ] = value;
            return true;
        }
        return false;
    }
    template < typename ValueType >
    bool set( const std::string& field, ValueType&& value ) {
        if( filter( field ) ) {
            m_dest[ field ] = std::move( value );
            return true;
        }
        return false;
    }

    bool filter( const std::string& field ) const {
        if( nullptr == m_filter || m_filter->emptyIncludeFields() ) {
            return true;
        }
        else if( m_filter->hasIncludeField( field ) ) {
            return true;
        }
        return false;
    }

private:
    JsonObject& m_dest;
    FilterDataListI* m_filter;
};

}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
