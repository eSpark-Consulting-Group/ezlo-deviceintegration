#include "cored/db_modifier/db_modifier.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {

class FieldModifier {
public:
    FieldModifier( DbModifier& dbModifier,
                   const JsonObject& newObject,
                   const JsonObject& oldObject,
                   const std::string& objectId )
    : m_dbModifier( dbModifier )
    , m_newObject( newObject )
    , m_oldObject( oldObject )
    , m_objectId( objectId ) {}

    template < typename ValueType >
    FieldModifier&& addField( const char* const fieldId ) && {
        ValueType field;
        if( json_get( m_newObject, fieldId, field ) ) {
            ValueType oldField;
            if( !json_get( m_oldObject, fieldId, oldField ) || field != oldField ) {
                m_jsonBuilder << fieldId << field;
            }
        }

        return std::move( *this );
    }

    void modifyFields() && {
        const auto query = m_jsonBuilder.getJson();
        if( !query.empty() ) {
            m_dbModifier.update( m_objectId, query, false );
        }
    }

private:
    DbModifier& m_dbModifier;

    const JsonObject& m_newObject;
    const JsonObject& m_oldObject;
    std::string m_objectId;

    JsonBuilder m_jsonBuilder;
};

}  // namespace CoreD
}  // namespace HubPlatform
