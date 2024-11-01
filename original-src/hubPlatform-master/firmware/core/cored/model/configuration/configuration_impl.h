#pragma once

#include "cored/model/configuration/configuration.h"
#include "database_client/idbclient.h"
#include "hub_settings/hub_setting.h"
#include <memory>

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace CoreD {
namespace Model {

class ConfigurationImpl : public Configuration {
public:
    explicit ConfigurationImpl( const IDbClient& dbClient ) noexcept;
    ~ConfigurationImpl() override;

    std::string getScaleForValueType( const ValueType& ) const override;

private:
    void readSettings( const IDbClient& dbClient );

private:
    std::unique_ptr< Hub::Setting > m_temperatureScaleUPtr;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
