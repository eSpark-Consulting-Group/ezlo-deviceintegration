#pragma once

#include "cored/db_modifier/db_modifier.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace CoreD {
namespace Model {
class ValueType;
class UiValueWriterVisitor;
}  // namespace Model

class GatewaySettingDbModifier final : public DbModifier {
public:
    GatewaySettingDbModifier( Services::Service& service, Broadcaster broadcaster );

    void insert( const Json& record ) override;
    void remove( const Id& recordId ) override;

    Json& prepareInsertBroadcastMessage( Json& broadcastData ) override;
    Json& prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord ) override;

private:
    void formatValue( Json& broadcastData,
                      const char* key,
                      const Model::ValueType& valueType,
                      Model::UiValueWriterVisitor& writer );

    Json& formatBroadcastValues( Json& broadcastData );

    void incrementGatewaySettings( const std::string& gatewayId );
    void decrementGatewaySettings( const Id& gatewaySettingId );

private:
    Services::Service& m_service;
};

}  // namespace CoreD
}  // namespace HubPlatform
