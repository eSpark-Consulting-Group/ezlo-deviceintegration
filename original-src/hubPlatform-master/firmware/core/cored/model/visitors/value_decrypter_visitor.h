#pragma once

#include "provision_data_utils/provision_data.h"
#include "cored/model/values/value_visitor.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ValueDecrypterVisitor : public ValueDefaultVisitor {
public:
    explicit ValueDecrypterVisitor( const ProvisionDataUtils::ProvisionData& provisionData );

    void visit( DictionaryValue& value ) override;
    void visit( UserCodeValue& value ) override;

private:
    const ProvisionDataUtils::ProvisionData& m_provisionData;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
