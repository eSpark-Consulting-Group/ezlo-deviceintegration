#include "cored/model/visitors/value_decrypter_visitor.h"

#include "cored/model/values/dictionary_value.h"
#include "cored/model/values/usercode_value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

ValueDecrypterVisitor::ValueDecrypterVisitor( const ProvisionDataUtils::ProvisionData& provisionData )
: m_provisionData( provisionData ) {}

void ValueDecrypterVisitor::visit( DictionaryValue& value ) {
    value.forEachValue(
        [&]( const DictionaryValue::ValueId& id, Value& elementValue ) { elementValue.accept( *this ); } );
}

void ValueDecrypterVisitor::visit( UserCodeValue& value ) {
    value.setUserCode( m_provisionData.decryptSensitiveData( value.getUserCode() ) );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
