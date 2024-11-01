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

class DeviceSettingDbModifier final : public DbModifier {
public:
    DeviceSettingDbModifier( Services::Service& service, Broadcaster broadcaster );

    ~DeviceSettingDbModifier() override;

    Json& prepareInsertBroadcastMessage( Json& broadcastData ) override;
    Json& prepareUpdateBroadcastMessage( Json& broadcastData );
    Json& prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord ) override;

    void sendUpdateBroadcast( MessageSubClasses subClass, Json& broadcastData, const Json& oldFieldsValues ) override;

    void updateDictionary( const Id& recordId, const Json& updatePortion, const Dictionary& dictionary );

private:
    void formatValue( Json& broadcastData,
                      const char* key,
                      const Model::ValueType& valueType,
                      Model::UiValueWriterVisitor& writer );

    Json& formatBroadcastValues( Json& broadcastData );

private:
    Services::Service& m_service;
    Dictionary m_dictionary;
};

}  // namespace CoreD
}  // namespace HubPlatform
