#include "cored/model/visitors/value_encrypter_visitor.h"

#include "cored/model/values/dictionary_value.h"
#include "cored/model/values/array_value.h"
#include "cored/model/values/usercode_value.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

ValueEncrypterVisitor::ValueEncrypterVisitor( const ProvisionDataUtils::ProvisionData& provisionData )
: m_provisionData( provisionData ) {}

void ValueEncrypterVisitor::visit( DictionaryValue& value ) {
    value.forEachValue(
        [this]( const DictionaryValue::ValueId&, Value& elementValue ) { elementValue.accept( *this ); } );
}

void ValueEncrypterVisitor::visit( ArrayValue& value ) {
    value.forEachValue( [this]( Value& elementValue ) { elementValue.accept( *this ); } );
}

void ValueEncrypterVisitor::visit( UserCodeValue& value ) {
    value.setUserCode( m_provisionData.encryptSensitiveData( value.getUserCode() ) );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
